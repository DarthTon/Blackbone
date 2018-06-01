// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../base/error.h"
#include "../base/globals.h"
#include "../base/intutil.h"
#include "../base/lock.h"
#include "../base/vmem.h"

// [Dependencies - Posix]
#if defined(ASMJIT_OS_POSIX)
# include <sys/types.h>
# include <sys/mman.h>
# include <unistd.h>
#endif // ASMJIT_OS_POSIX

// [Api-Begin]
#include "../apibegin.h"

// This file contains implementation of virtual memory management for AsmJit
// library. The initial concept is to keep this implementation simple but
// efficient. There are several goals I decided to write implementation myself.
//
// Goals:
//
// - Granularity of allocated blocks is different than granularity for a typical
//   C malloc. It is at least 64-bytes so Assembler/Compiler can guarantee the
//   alignment required. Alignment requirements can grow in the future, but at
//   the moment 64 bytes is safe (we may jump to 128 bytes if necessary or make
//   it configurable).
//
// - Keep memory manager information outside of the allocated virtual memory
//   pages, because these pages allow executing of machine code and there should
//   not be data required to keep track of these blocks. Another reason is that
//   some environments (i.e. iOS) allow to generate and run JIT code, but this
//   code has to be set to [Executable, but not Writable].
//
// - Keep implementation simple and easy to follow.
//
// Implementation is based on bit arrays and binary trees. Bit arrays contain
// information related to allocated and unused blocks of memory. The size of
// a block is described by `MemNode::density`. Count of blocks is stored in
// `MemNode::blocks`. For example if density is 64 and count of blocks is 20,
// memory node contains 64*20 bytes of memory and smallest possible allocation
// (and also alignment) is 64 bytes. So density is also related to memory
// alignment. Binary trees (RB) are used to enable fast lookup into all addresses
// allocated by memory manager instance. This is used mainly by `VMemPrivate::release()`.
//
// Bit array looks like this (empty = unused, X = used) - Size of block 64:
//
//   -------------------------------------------------------------------------
//   | |X|X| | | | | |X|X|X|X|X|X| | | | | | | | | | | | |X| | | | |X|X|X| | |
//   -------------------------------------------------------------------------
//                               (Maximum continuous block)
//
// These bits show that there are 12 allocated blocks (X) of 64 bytes, so total
// size allocated is 768 bytes. Maximum count of continuous memory is 12 * 64.

namespace asmjit {

// ============================================================================
// [asmjit::VMemUtil - Windows]
// ============================================================================

// Windows specific implementation using `VirtualAllocEx` and `VirtualFree`.
#if defined(ASMJIT_OS_WINDOWS)
struct VMemLocal {
  // AsmJit allows to pass a `NULL` handle to `VMemUtil`. This function is just
  // a convenient way to convert such handle to the current process one.
  ASMJIT_INLINE HANDLE getSafeProcessHandle(HANDLE hParam) const {
    return hParam != NULL ? hParam : hProcess;
  }

  size_t pageSize;
  size_t pageGranularity;
  HANDLE hProcess;
};
static VMemLocal vMemLocal;

static const VMemLocal& vMemGet() {
  VMemLocal& vMem = vMemLocal;

  if (!vMem.hProcess) {
    SYSTEM_INFO info;
    ::GetSystemInfo(&info);

    vMem.pageSize = IntUtil::alignToPowerOf2<uint32_t>(info.dwPageSize);
    vMem.pageGranularity = info.dwAllocationGranularity;

    vMem.hProcess = ::GetCurrentProcess();
  }

  return vMem;
};

size_t VMemUtil::getPageSize() {
  const VMemLocal& vMem = vMemGet();
  return vMem.pageSize;
}

size_t VMemUtil::getPageGranularity() {
  const VMemLocal& vMem = vMemGet();
  return vMem.pageGranularity;
}

void* VMemUtil::alloc(size_t length, size_t* allocated, uint32_t flags) {
  return allocProcessMemory(static_cast<HANDLE>(0), length, allocated, flags);
}

void* VMemUtil::allocProcessMemory(HANDLE hProcess, size_t length, size_t* allocated, uint32_t flags) {
  if (length == 0)
    return NULL;

  const VMemLocal& vMem = vMemGet();
  hProcess = vMem.getSafeProcessHandle(hProcess);

  // VirtualAlloc rounds allocated size to a page size automatically.
  size_t mSize = IntUtil::alignTo(length, vMem.pageSize);

  // Windows XP SP2 / Vista allow Data Excution Prevention (DEP).
  DWORD protectFlags = 0;

  if (flags & kVMemFlagExecutable)
    protectFlags |= (flags & kVMemFlagWritable) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
  else
    protectFlags |= (flags & kVMemFlagWritable) ? PAGE_READWRITE : PAGE_READONLY;

  LPVOID mBase = ::VirtualAllocEx(hProcess, NULL, mSize, MEM_COMMIT | MEM_RESERVE, protectFlags);
  if (mBase == NULL)
    return NULL;

  ASMJIT_ASSERT(IntUtil::isAligned<size_t>(
    reinterpret_cast<size_t>(mBase), vMem.pageSize));

  if (allocated != NULL)
    *allocated = mSize;
  return mBase;
}

Error VMemUtil::release(void* addr, size_t length) {
  return releaseProcessMemory(static_cast<HANDLE>(0), addr, length);
}

Error VMemUtil::releaseProcessMemory(HANDLE hProcess, void* addr, size_t /* length */) {
  hProcess = vMemGet().getSafeProcessHandle(hProcess);
  if (!::VirtualFreeEx(hProcess, addr, 0, MEM_RELEASE))
    return kErrorInvalidState;
  return kErrorOk;
}
#endif // ASMJIT_OS_WINDOWS

// ============================================================================
// [asmjit::VMemUtil - Posix]
// ============================================================================

// Posix specific implementation using `mmap` and `munmap`.
#if defined(ASMJIT_OS_POSIX)

// MacOS uses MAP_ANON instead of MAP_ANONYMOUS.
#if !defined(MAP_ANONYMOUS)
# define MAP_ANONYMOUS MAP_ANON
#endif // MAP_ANONYMOUS

struct VMemLocal {
  size_t pageSize;
  size_t pageGranularity;
};
static VMemLocal vMemLocal;

static const VMemLocal& vMemGet() {
  VMemLocal& vMem = vMemLocal;

  if (!vMem.pageSize) {
    size_t pageSize = ::getpagesize();
    vMem.pageSize = pageSize;
    vMem.pageGranularity = IntUtil::iMax<size_t>(pageSize, 65536);
  }

  return vMem;
};

size_t VMemUtil::getPageSize() {
  const VMemLocal& vMem = vMemGet();
  return vMem.pageSize;
}

size_t VMemUtil::getPageGranularity() {
  const VMemLocal& vMem = vMemGet();
  return vMem.pageGranularity;
}

void* VMemUtil::alloc(size_t length, size_t* allocated, uint32_t flags) {
  const VMemLocal& vMem = vMemGet();
  size_t msize = IntUtil::alignTo<size_t>(length, vMem.pageSize);
  int protection = PROT_READ;

  if (flags & kVMemFlagWritable  ) protection |= PROT_WRITE;
  if (flags & kVMemFlagExecutable) protection |= PROT_EXEC;

  void* mbase = ::mmap(NULL, msize, protection, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mbase == MAP_FAILED)
    return NULL;

  if (allocated != NULL)
    *allocated = msize;
  return mbase;
}

Error VMemUtil::release(void* addr, size_t length) {
  if (::munmap(addr, length) != 0)
    return kErrorInvalidState;

  return kErrorOk;
}
#endif // ASMJIT_OS_POSIX

// ============================================================================
// [asmjit::VMemMgr - BitOps]
// ============================================================================

#define M_DIV(x, y) ((x) / (y))
#define M_MOD(x, y) ((x) % (y))

//! \internal
enum {
  kBitsPerEntity = (sizeof(size_t) * 8)
};

//! \internal
//!
//! Set `len` bits in `buf` starting at `index` bit index.
static void _SetBits(size_t* buf, size_t index, size_t len) {
  if (len == 0)
    return;

  size_t i = index / kBitsPerEntity; // size_t[]
  size_t j = index % kBitsPerEntity; // size_t[][] bit index

  // How many bytes process in the first group.
  size_t c = kBitsPerEntity - j;
  if (c > len)
    c = len;

  // Offset.
  buf += i;

  *buf++ |= ((~(size_t)0) >> (kBitsPerEntity - c)) << j;
  len -= c;

  while (len >= kBitsPerEntity) {
    *buf++ = ~(size_t)0;
    len -= kBitsPerEntity;
  }

  if (len)
    *buf |= ((~(size_t)0) >> (kBitsPerEntity - len));
}

// ============================================================================
// [asmjit::VMemMgr::TypeDefs]
// ============================================================================

typedef VMemMgr::RbNode RbNode;
typedef VMemMgr::MemNode MemNode;
typedef VMemMgr::PermanentNode PermanentNode;

// ============================================================================
// [asmjit::VMemMgr::RbNode]
// ============================================================================

//! \internal
//!
//! Base red-black tree node.
struct VMemMgr::RbNode {
  // Implementation is based on article by Julienne Walker (Public Domain),
  // including C code and original comments. Thanks for the excellent article.

  // Left[0] and right[1] nodes.
  RbNode* node[2];
  // Virtual memory address.
  uint8_t* mem;
  // Whether the node is RED.
  uint32_t red;
};

//! \internal
//!
//! Get whether the node is red (NULL or node with red flag).
static ASMJIT_INLINE bool rbIsRed(RbNode* node) {
  return node != NULL && node->red;
}

//! \internal
//!
//! Check whether the RB tree is valid.
static int rbAssert(RbNode* root) {
  if (root == NULL)
    return 1;

  RbNode* ln = root->node[0];
  RbNode* rn = root->node[1];

  // Red violation.
  ASMJIT_ASSERT( !(rbIsRed(root) && (rbIsRed(ln) || rbIsRed(rn))) );

  int lh = rbAssert(ln);
  int rh = rbAssert(rn);

  // Invalid btree.
  ASMJIT_ASSERT(ln == NULL || ln->mem < root->mem);
  ASMJIT_ASSERT(rn == NULL || rn->mem > root->mem);

  // Black violation.
  ASMJIT_ASSERT( !(lh != 0 && rh != 0 && lh != rh) );

  // Only count black links.
  if (lh != 0 && rh != 0)
    return rbIsRed(root) ? lh : lh + 1;
  else
    return 0;
}

//! \internal
//!
//! Single rotation.
static ASMJIT_INLINE RbNode* rbRotateSingle(RbNode* root, int dir) {
  RbNode* save = root->node[!dir];

  root->node[!dir] = save->node[dir];
  save->node[dir] = root;

  root->red = 1;
  save->red = 0;

  return save;
}

//! \internal
//!
//! Double rotation.
static ASMJIT_INLINE RbNode* rbRotateDouble(RbNode* root, int dir) {
  root->node[!dir] = rbRotateSingle(root->node[!dir], !dir);
  return rbRotateSingle(root, dir);
}

// ============================================================================
// [asmjit::VMemMgr::MemNode]
// ============================================================================

struct VMemMgr::MemNode : public RbNode {
  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  // Get available space.
  ASMJIT_INLINE size_t getAvailable() const {
    return size - used;
  }

  ASMJIT_INLINE void fillData(MemNode* other) {
    mem = other->mem;

    size = other->size;
    used = other->used;
    blocks = other->blocks;
    density = other->density;
    largestBlock = other->largestBlock;

    baUsed = other->baUsed;
    baCont = other->baCont;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  MemNode* prev;         // Prev node in list.
  MemNode* next;         // Next node in list.

  size_t size;           // How many bytes contain this node.
  size_t used;           // How many bytes are used in this node.
  size_t blocks;         // How many blocks are here.
  size_t density;        // Minimum count of allocated bytes in this node (also alignment).
  size_t largestBlock;   // Contains largest block that can be allocated.

  size_t* baUsed;        // Contains bits about used blocks       (0 = unused, 1 = used).
  size_t* baCont;        // Contains bits about continuous blocks (0 = stop  , 1 = continue).
};

// ============================================================================
// [asmjit::VMemMgr::PermanentNode]
// ============================================================================

//! \internal
//!
//! Permanent node.
struct VMemMgr::PermanentNode {
  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  //! Get available space.
  ASMJIT_INLINE size_t getAvailable() const {
    return size - used;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  PermanentNode* prev;   // Pointer to prev chunk or NULL.
  uint8_t* mem;          // Base pointer (virtual memory address).
  size_t size;           // Count of bytes allocated.
  size_t used;           // Count of bytes used.
};

// ============================================================================
// [asmjit::VMemMgr - Private]
// ============================================================================

//! \internal
//!
//! Helper to avoid `#ifdef`s in the code.
ASMJIT_INLINE uint8_t* vMemMgrAllocVMem(VMemMgr* self, size_t size, size_t* vSize) {
  uint32_t flags = kVMemFlagWritable | kVMemFlagExecutable;
#if !defined(ASMJIT_OS_WINDOWS)
  return static_cast<uint8_t*>(VMemUtil::alloc(size, vSize, flags));
#else
  return static_cast<uint8_t*>(VMemUtil::allocProcessMemory(self->_hProcess, size, vSize, flags));
#endif
}

//! \internal
//!
//! Helper to avoid `#ifdef`s in the code.
ASMJIT_INLINE Error vMemMgrReleaseVMem(VMemMgr* self, void* p, size_t vSize) {
#if !defined(ASMJIT_OS_WINDOWS)
  return VMemUtil::release(p, vSize);
#else
  return VMemUtil::releaseProcessMemory(self->_hProcess, p, vSize);
#endif
}

//! \internal
//!
//! Check whether the Red-Black tree is valid.
static bool vMemMgrCheckTree(VMemMgr* self) {
  return rbAssert(self->_root) > 0;
}

//! \internal
//!
//! Alloc virtual memory including a heap memory needed for `MemNode` data.
//!
//! Returns set-up `MemNode*` or NULL if allocation failed.
static MemNode* vMemMgrCreateNode(VMemMgr* self, size_t size, size_t density) {
  size_t vSize;
  uint8_t* vmem = vMemMgrAllocVMem(self, size, &vSize);

  // Out of memory.
  if (vmem == NULL)
    return NULL;

  size_t blocks = (vSize / density);
  size_t bsize = (((blocks + 7) >> 3) + sizeof(size_t) - 1) & ~(size_t)(sizeof(size_t) - 1);

  MemNode* node = static_cast<MemNode*>(ASMJIT_ALLOC(sizeof(MemNode)));
  uint8_t* data = static_cast<uint8_t*>(ASMJIT_ALLOC(bsize * 2));

  // Out of memory.
  if (node == NULL || data == NULL) {
    vMemMgrReleaseVMem(self, vmem, vSize);
    if (node) ASMJIT_FREE(node);
    if (data) ASMJIT_FREE(data);
    return NULL;
  }

  // Initialize RbNode data.
  node->node[0] = NULL;
  node->node[1] = NULL;
  node->mem = vmem;
  node->red = 1;

  // Initialize MemNode data.
  node->prev = NULL;
  node->next = NULL;

  node->size = vSize;
  node->used = 0;
  node->blocks = blocks;
  node->density = density;
  node->largestBlock = vSize;

  ::memset(data, 0, bsize * 2);
  node->baUsed = reinterpret_cast<size_t*>(data);
  node->baCont = reinterpret_cast<size_t*>(data + bsize);

  return node;
}

static void vMemMgrInsertNode(VMemMgr* self, MemNode* node) {
  if (self->_root == NULL) {
    // Empty tree case.
    self->_root = node;
  }
  else {
    // False tree root.
    RbNode head = { { 0 } };

    // Grandparent & parent.
    RbNode* g = NULL;
    RbNode* t = &head;

    // Iterator & parent.
    RbNode* p = NULL;
    RbNode* q = t->node[1] = self->_root;

    int dir = 0, last = 0;

    // Search down the tree.
    for (;;) {
      if (q == NULL) {
        // Insert new node at the bottom.
        q = node;
        p->node[dir] = node;
      }
      else if (rbIsRed(q->node[0]) && rbIsRed(q->node[1])) {
        // Color flip.
        q->red = 1;
        q->node[0]->red = 0;
        q->node[1]->red = 0;
      }

      // Fix red violation.
      if (rbIsRed(q) && rbIsRed(p)) {
        int dir2 = t->node[1] == g;
        t->node[dir2] = q == p->node[last] ? rbRotateSingle(g, !last) : rbRotateDouble(g, !last);
      }

      // Stop if found.
      if (q == node)
        break;

      last = dir;
      dir = q->mem < node->mem;

      // Update helpers.
      if (g != NULL)
        t = g;

      g = p;
      p = q;
      q = q->node[dir];
    }

    // Update root.
    self->_root = static_cast<MemNode*>(head.node[1]);
  }

  // Make root black.
  self->_root->red = 0;

  // Link with others.
  node->prev = self->_last;

  if (self->_first == NULL) {
    self->_first = node;
    self->_last = node;
    self->_optimal = node;
  }
  else {
    node->prev = self->_last;
    self->_last->next = node;
    self->_last = node;
  }
}

//! \internal
//!
//! Remove node from Red-Black tree.
//!
//! Returns node that should be freed, but it doesn't have to be necessarily
//! the `node` passed.
static MemNode* vMemMgrRemoveNode(VMemMgr* self, MemNode* node) {
  // False tree root.
  RbNode head = { { 0 } };

  // Helpers.
  RbNode* q = &head;
  RbNode* p = NULL;
  RbNode* g = NULL;

  // Found item.
  RbNode* f = NULL;
  int dir = 1;

  // Set up.
  q->node[1] = self->_root;

  // Search and push a red down.
  while (q->node[dir] != NULL) {
    int last = dir;

    // Update helpers.
    g = p;
    p = q;
    q = q->node[dir];
    dir = q->mem < node->mem;

    // Save found node.
    if (q == node)
      f = q;

    // Push the red node down.
    if (!rbIsRed(q) && !rbIsRed(q->node[dir])) {
      if (rbIsRed(q->node[!dir])) {
        p = p->node[last] = rbRotateSingle(q, dir);
      }
      else if (!rbIsRed(q->node[!dir])) {
        RbNode* s = p->node[!last];

        if (s != NULL) {
          if (!rbIsRed(s->node[!last]) && !rbIsRed(s->node[last])) {
            // Color flip.
            p->red = 0;
            s->red = 1;
            q->red = 1;
          }
          else {
            int dir2 = g->node[1] == p;

            if (rbIsRed(s->node[last]))
              g->node[dir2] = rbRotateDouble(p, last);
            else if (rbIsRed(s->node[!last]))
              g->node[dir2] = rbRotateSingle(p, last);

            // Ensure correct coloring.
            q->red = g->node[dir2]->red = 1;
            g->node[dir2]->node[0]->red = 0;
            g->node[dir2]->node[1]->red = 0;
          }
        }
      }
    }
  }

  // Replace and remove.
  ASMJIT_ASSERT(f != NULL);
  ASMJIT_ASSERT(f != &head);
  ASMJIT_ASSERT(q != &head);

  if (f != q) {
    ASMJIT_ASSERT(f != &head);
    static_cast<MemNode*>(f)->fillData(static_cast<MemNode*>(q));
  }

  p->node[p->node[1] == q] = q->node[q->node[0] == NULL];

  // Update root and make it black.
  self->_root = static_cast<MemNode*>(head.node[1]);
  if (self->_root != NULL)
    self->_root->red = 0;

  // Unlink.
  MemNode* next = static_cast<MemNode*>(q)->next;
  MemNode* prev = static_cast<MemNode*>(q)->prev;

  if (prev)
    prev->next = next;
  else
    self->_first = next;

  if (next)
    next->prev = prev;
  else
    self->_last  = prev;

  if (self->_optimal == q)
    self->_optimal = prev ? prev : next;

  return static_cast<MemNode*>(q);
}

static MemNode* vMemMgrFindNodeByPtr(VMemMgr* self, uint8_t* mem) {
  MemNode* node = self->_root;
  while (node != NULL) {
    uint8_t* nodeMem = node->mem;

    // Go left.
    if (mem < nodeMem) {
      node = static_cast<MemNode*>(node->node[0]);
      continue;
    }

    // Go right.
    uint8_t* nodeEnd = nodeMem + node->size;
    if (mem >= nodeEnd) {
      node = static_cast<MemNode*>(node->node[1]);
      continue;
    }

    // Match.
    break;
  }
  return node;
}

static void* vMemMgrAllocPermanent(VMemMgr* self, size_t vSize) {
  static const size_t permanentAlignment = 32;
  static const size_t permanentNodeSize  = 32768;

  vSize = IntUtil::alignTo<size_t>(vSize, permanentAlignment);

  AutoLock locked(self->_lock);
  PermanentNode* node = self->_permanent;

  // Try to find space in allocated chunks.
  while (node && vSize > node->getAvailable())
    node = node->prev;

  // Or allocate new node.
  if (node == NULL) {
    size_t nodeSize = permanentNodeSize;

    if (nodeSize < vSize)
      nodeSize = vSize;

    node = static_cast<PermanentNode*>(ASMJIT_ALLOC(sizeof(PermanentNode)));

    // Out of memory.
    if (node == NULL)
      return NULL;

    node->mem = vMemMgrAllocVMem(self, nodeSize, &node->size);

    // Out of memory.
    if (node->mem == NULL) {
      ASMJIT_FREE(node);
      return NULL;
    }

    node->used = 0;
    node->prev = self->_permanent;
    self->_permanent = node;
  }

  // Finally, copy function code to our space we reserved for.
  uint8_t* result = node->mem + node->used;

  // Update Statistics.
  node->used += vSize;
  self->_usedBytes += vSize;

  // Code can be null to only reserve space for code.
  return static_cast<void*>(result);
}

static void* vMemMgrAllocFreeable(VMemMgr* self, size_t vSize) {
  // Current index.
  size_t i;

  // How many we need to be freed.
  size_t need;
  size_t minVSize;

  // Align to 32 bytes by default.
  vSize = IntUtil::alignTo<size_t>(vSize, 32);
  if (vSize == 0)
    return NULL;

  AutoLock locked(self->_lock);
  MemNode* node = self->_optimal;
  minVSize = self->_blockSize;

  // Try to find memory block in existing nodes.
  while (node) {
    // Skip this node?
    if ((node->getAvailable() < vSize) || (node->largestBlock < vSize && node->largestBlock != 0)) {
      MemNode* next = node->next;

      if (node->getAvailable() < minVSize && node == self->_optimal && next)
        self->_optimal = next;

      node = next;
      continue;
    }

    size_t* up = node->baUsed;     // Current ubits address.
    size_t ubits;                  // Current ubits[0] value.
    size_t bit;                    // Current bit mask.
    size_t blocks = node->blocks;  // Count of blocks in node.
    size_t cont = 0;               // How many bits are currently freed in find loop.
    size_t maxCont = 0;            // Largest continuous block (bits count).
    size_t j;

    need = M_DIV((vSize + node->density - 1), node->density);
    i = 0;

    // Try to find node that is large enough.
    while (i < blocks) {
      ubits = *up++;

      // Fast skip used blocks.
      if (ubits == ~(size_t)0) {
        if (cont > maxCont)
          maxCont = cont;
        cont = 0;

        i += kBitsPerEntity;
        continue;
      }

      size_t max = kBitsPerEntity;
      if (i + max > blocks)
        max = blocks - i;

      for (j = 0, bit = 1; j < max; bit <<= 1) {
        j++;
        if ((ubits & bit) == 0) {
          if (++cont == need) {
            i += j;
            i -= cont;
            goto _Found;
          }

          continue;
        }

        if (cont > maxCont) maxCont = cont;
        cont = 0;
      }

      i += kBitsPerEntity;
    }

    // Because we traversed the entire node, we can set largest node size that
    // will be used to cache next traversing.
    node->largestBlock = maxCont * node->density;

    node = node->next;
  }

  // If we are here, we failed to find existing memory block and we must
  // allocate a new one.
  {
    size_t blockSize = self->_blockSize;
    if (blockSize < vSize)
      blockSize = vSize;

    node = vMemMgrCreateNode(self, blockSize, self->_blockDensity);
    if (node == NULL)
      return NULL;

    // Update binary tree.
    vMemMgrInsertNode(self, node);
    ASMJIT_ASSERT(vMemMgrCheckTree(self));

    // Alloc first node at start.
    i = 0;
    need = (vSize + node->density - 1) / node->density;

    // Update statistics.
    self->_allocatedBytes += node->size;
  }

_Found:
  // Update bits.
  _SetBits(node->baUsed, i, need);
  _SetBits(node->baCont, i, need - 1);

  // Update statistics.
  {
    size_t u = need * node->density;
    node->used += u;
    node->largestBlock = 0;
    self->_usedBytes += u;
  }

  // And return pointer to allocated memory.
  uint8_t* result = node->mem + i * node->density;
  ASMJIT_ASSERT(result >= node->mem && result <= node->mem + node->size - vSize);
  return result;
}

//! \internal
//!
//! Reset the whole `VMemMgr` instance, freeing all heap memory allocated an
//! virtual memory allocated unless `keepVirtualMemory` is true (and this is
//! only used when writing data to a remote process).
static void vMemMgrReset(VMemMgr* self, bool keepVirtualMemory) {
  MemNode* node = self->_first;

  while (node != NULL) {
    MemNode* next = node->next;

    if (!keepVirtualMemory)
      vMemMgrReleaseVMem(self, node->mem, node->size);

    ASMJIT_FREE(node->baUsed);
    ASMJIT_FREE(node);

    node = next;
  }

  self->_allocatedBytes = 0;
  self->_usedBytes = 0;

  self->_root = NULL;
  self->_first = NULL;
  self->_last = NULL;
  self->_optimal = NULL;
}

// ============================================================================
// [asmjit::VMemMgr - Construction / Destruction]
// ============================================================================

#if !defined(ASMJIT_OS_WINDOWS)
VMemMgr::VMemMgr()
#else
VMemMgr::VMemMgr(HANDLE hProcess) :
  _hProcess(vMemGet().getSafeProcessHandle(hProcess))
#endif // ASMJIT_OS_WINDOWS
{

  _blockSize = VMemUtil::getPageGranularity();
  _blockDensity = 64;

  _allocatedBytes = 0;
  _usedBytes = 0;

  _root = NULL;
  _first = NULL;
  _last = NULL;
  _optimal = NULL;

  _permanent = NULL;
  _keepVirtualMemory = false;
}

VMemMgr::~VMemMgr() {
  // Freeable memory cleanup - Also frees the virtual memory if configured to.
  vMemMgrReset(this, _keepVirtualMemory);

  // Permanent memory cleanup - Never frees the virtual memory.
  PermanentNode* node = _permanent;
  while (node) {
    PermanentNode* prev = node->prev;
    ASMJIT_FREE(node);
    node = prev;
  }
}

// ============================================================================
// [asmjit::VMemMgr - Reset]
// ============================================================================

void VMemMgr::reset() {
  vMemMgrReset(this, false);
}

// ============================================================================
// [asmjit::VMemMgr - Alloc / Release]
// ============================================================================

void* VMemMgr::alloc(size_t size, uint32_t type) {
  if (type == kVMemAllocPermanent)
    return vMemMgrAllocPermanent(this, size);
  else
    return vMemMgrAllocFreeable(this, size);
}

Error VMemMgr::release(void* p) {
  if (p == NULL)
    return kErrorOk;

  AutoLock locked(_lock);
  MemNode* node = vMemMgrFindNodeByPtr(this, static_cast<uint8_t*>(p));

  if (node == NULL)
    return kErrorInvalidArgument;

  size_t offset = (size_t)((uint8_t*)p - (uint8_t*)node->mem);
  size_t bitpos = M_DIV(offset, node->density);
  size_t i = (bitpos / kBitsPerEntity);

  size_t* up = node->baUsed + i;  // Current ubits address.
  size_t* cp = node->baCont + i;  // Current cbits address.
  size_t ubits = *up;             // Current ubits[0] value.
  size_t cbits = *cp;             // Current cbits[0] value.
  size_t bit = (size_t)1 << (bitpos % kBitsPerEntity);

  size_t cont = 0;
  bool stop;

  for (;;) {
    stop = (cbits & bit) == 0;
    ubits &= ~bit;
    cbits &= ~bit;

    bit <<= 1;
    cont++;

    if (stop || bit == 0) {
      *up = ubits;
      *cp = cbits;
      if (stop)
        break;

      ubits = *++up;
      cbits = *++cp;
      bit = 1;
    }
  }

  // If the freed block is fully allocated node then it's needed to
  // update 'optimal' pointer in memory manager.
  if (node->used == node->size) {
    MemNode* cur = _optimal;

    do {
      cur = cur->prev;
      if (cur == node) {
        _optimal = node;
        break;
      }
    } while (cur);
  }

  // Statistics.
  cont *= node->density;
  if (node->largestBlock < cont)
    node->largestBlock = cont;

  node->used -= cont;
  _usedBytes -= cont;

  // If page is empty, we can free it.
  if (node->used == 0) {
    // Free memory associated with node (this memory is not accessed
    // anymore so it's safe).
    vMemMgrReleaseVMem(this, node->mem, node->size);
    ASMJIT_FREE(node->baUsed);

    node->baUsed = NULL;
    node->baCont = NULL;

    // Statistics.
    _allocatedBytes -= node->size;

    // Remove node. This function can return different node than
    // passed into, but data is copied into previous node if needed.
    ASMJIT_FREE(vMemMgrRemoveNode(this, node));
    ASMJIT_ASSERT(vMemMgrCheckTree(this));
  }

  return kErrorOk;
}

Error VMemMgr::shrink(void* p, size_t used) {
  if (p == NULL)
    return kErrorOk;

  if (used == 0)
    return release(p);

  AutoLock locked(_lock);

  MemNode* node = vMemMgrFindNodeByPtr(this, (uint8_t*)p);
  if (node == NULL)
    return kErrorInvalidArgument;

  size_t offset = (size_t)((uint8_t*)p - (uint8_t*)node->mem);
  size_t bitpos = M_DIV(offset, node->density);
  size_t i = (bitpos / kBitsPerEntity);

  size_t* up = node->baUsed + i;  // Current ubits address.
  size_t* cp = node->baCont + i;  // Current cbits address.
  size_t ubits = *up;             // Current ubits[0] value.
  size_t cbits = *cp;             // Current cbits[0] value.
  size_t bit = (size_t)1 << (bitpos % kBitsPerEntity);

  size_t cont = 0;
  size_t usedBlocks = (used + node->density - 1) / node->density;

  bool stop;

  // Find the first block we can mark as free.
  for (;;) {
    stop = (cbits & bit) == 0;
    if (stop)
      return kErrorOk;

    if (++cont == usedBlocks)
      break;

    bit <<= 1;
    if (bit == 0) {
      ubits = *++up;
      cbits = *++cp;
      bit = 1;
    }
  }

  // Free the tail blocks.
  cont = ~(size_t)0;
  goto _EnterFreeLoop;

  for (;;) {
    stop = (cbits & bit) == 0;
    ubits &= ~bit;

_EnterFreeLoop:
    cbits &= ~bit;

    bit <<= 1;
    cont++;

    if (stop || bit == 0) {
      *up = ubits;
      *cp = cbits;
      if (stop)
        break;

      ubits = *++up;
      cbits = *++cp;
      bit = 1;
    }
  }

  // Statistics.
  cont *= node->density;
  if (node->largestBlock < cont)
    node->largestBlock = cont;

  node->used -= cont;
  _usedBytes -= cont;

  return kErrorOk;
}

// ============================================================================
// [asmjit::VMem - Test]
// ============================================================================

#if defined(ASMJIT_TEST)
static void VMemTest_fill(void* a, void* b, int i) {
  int pattern = rand() % 256;
  *(int *)a = i;
  *(int *)b = i;
  ::memset((char*)a + sizeof(int), pattern, i - sizeof(int));
  ::memset((char*)b + sizeof(int), pattern, i - sizeof(int));
}

static void VMemTest_verify(void* a, void* b) {
  int ai = *(int*)a;
  int bi = *(int*)b;

  EXPECT(ai == bi,
    "The length of 'a' (%d) and 'b' (%d) should be same", ai, bi);

  EXPECT(::memcmp(a, b, ai) == 0,
    "Pattern (%p) doesn't match", a);
}

static void VMemTest_stats(VMemMgr& memmgr) {
  INFO("Used     : %u", static_cast<unsigned int>(memmgr.getUsedBytes()));
  INFO("Allocated: %u", static_cast<unsigned int>(memmgr.getAllocatedBytes()));
}

static void VMemTest_shuffle(void **a, void **b, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    size_t si = (size_t)rand() % count;

    void *ta = a[i];
    void *tb = b[i];

    a[i] = a[si];
    b[i] = b[si];

    a[si] = ta;
    b[si] = tb;
  }
}

UNIT(base_vmem) {
  VMemMgr memmgr;

  // Should be predictible.
  srand(100);

  int i;
  int kCount = 200000;

  INFO("Memory alloc/free test - %d allocations.", static_cast<int>(kCount));

  void** a = (void**)ASMJIT_ALLOC(sizeof(void*) * kCount);
  void** b = (void**)ASMJIT_ALLOC(sizeof(void*) * kCount);

  EXPECT(a != NULL && b != NULL,
    "Couldn't allocate %u bytes on heap.", kCount * 2);

  INFO("Allocating virtual memory...");
  for (i = 0; i < kCount; i++) {
    int r = (rand() % 1000) + 4;

    a[i] = memmgr.alloc(r);
    EXPECT(a[i] != NULL,
      "Couldn't allocate %d bytes of virtual memory", r);
    ::memset(a[i], 0, r);
  }
  VMemTest_stats(memmgr);

  INFO("Freeing virtual memory...");
  for (i = 0; i < kCount; i++) {
    EXPECT(memmgr.release(a[i]) == kErrorOk,
      "Failed to free %p.", b[i]);
  }
  VMemTest_stats(memmgr);

  INFO("Verified alloc/free test - %d allocations.", static_cast<int>(kCount));
  for (i = 0; i < kCount; i++) {
    int r = (rand() % 1000) + 4;

    a[i] = memmgr.alloc(r);
    EXPECT(a[i] != NULL,
      "Couldn't allocate %d bytes of virtual memory.", r);

    b[i] = ASMJIT_ALLOC(r);
    EXPECT(b[i] != NULL,
      "Couldn't allocate %d bytes on heap.", r);

    VMemTest_fill(a[i], b[i], r);
  }
  VMemTest_stats(memmgr);

  INFO("Shuffling...");
  VMemTest_shuffle(a, b, kCount);

  INFO("Verify and free...");
  for (i = 0; i < kCount / 2; i++) {
    VMemTest_verify(a[i], b[i]);
    EXPECT(memmgr.release(a[i]) == kErrorOk,
      "Failed to free %p.", a[i]);
    ASMJIT_FREE(b[i]);
  }
  VMemTest_stats(memmgr);

  INFO("Alloc again.");
  for (i = 0; i < kCount / 2; i++) {
    int r = (rand() % 1000) + 4;

    a[i] = memmgr.alloc(r);
    EXPECT(a[i] != NULL,
      "Couldn't allocate %d bytes of virtual memory.", r);

    b[i] = ASMJIT_ALLOC(r);
    EXPECT(b[i] != NULL,
      "Couldn't allocate %d bytes on heap.");

    VMemTest_fill(a[i], b[i], r);
  }
  VMemTest_stats(memmgr);

  INFO("Verify and free...");
  for (i = 0; i < kCount; i++) {
    VMemTest_verify(a[i], b[i]);
    EXPECT(memmgr.release(a[i]) == kErrorOk,
      "Failed to free %p.", a[i]);
    ASMJIT_FREE(b[i]);
  }
  VMemTest_stats(memmgr);

  ASMJIT_FREE(a);
  ASMJIT_FREE(b);
}
#endif // ASMJIT_TEST

} // asmjit namespace
