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
  VMemLocal() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    hProcess = GetCurrentProcess();
    alignment = info.dwAllocationGranularity;
    pageSize = IntUtil::alignToPowerOf2<uint32_t>(info.dwPageSize);
  }

  HANDLE hProcess;
  size_t alignment;
  size_t pageSize;
};

static VMemLocal& vm() {
  static VMemLocal vm;
  return vm;
};

size_t VMemUtil::getAlignment() {
  return vm().alignment;
}

size_t VMemUtil::getPageSize() {
  return vm().pageSize;
}

void* VMemUtil::alloc(size_t length, size_t* allocated, uint32_t flags) {
  return allocProcessMemory(static_cast<HANDLE>(0), length, allocated, flags);
}

void VMemUtil::release(void* addr, size_t length) {
  return releaseProcessMemory(static_cast<HANDLE>(0), addr, length);
}

void* VMemUtil::allocProcessMemory(HANDLE hProcess, size_t length, size_t* allocated, uint32_t flags) {
  VMemLocal& vmLocal = vm();

  if (hProcess == static_cast<HANDLE>(0))
    hProcess = vmLocal.hProcess;

  // VirtualAlloc rounds allocated size to a page size automatically.
  size_t mSize = IntUtil::alignTo(length, vmLocal.pageSize);

  // Windows XP SP2 / Vista allow Data Excution Prevention (DEP).
  DWORD protectFlags = 0;

  if (flags & kVMemFlagExecutable) {
    protectFlags |= (flags & kVMemFlagWritable)
      ? PAGE_EXECUTE_READWRITE
      : PAGE_EXECUTE_READ;
  }
  else {
    protectFlags |= (flags & kVMemFlagWritable)
      ? PAGE_READWRITE
      : PAGE_READONLY;
  }

  LPVOID mBase = VirtualAllocEx(hProcess, NULL, mSize, MEM_COMMIT | MEM_RESERVE, protectFlags);
  if (mBase == NULL)
    return NULL;

  ASMJIT_ASSERT(IntUtil::isAligned<size_t>(
    reinterpret_cast<size_t>(mBase), vmLocal.alignment));

  if (allocated != NULL)
    *allocated = mSize;
  return mBase;
}

void VMemUtil::releaseProcessMemory(HANDLE hProcess, void* addr, size_t /* length */) {
  VirtualFreeEx(hProcess, addr, 0, MEM_RELEASE);
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
  VMemLocal() {
    alignment = pageSize = ::getpagesize();
  }

  size_t alignment;
  size_t pageSize;
};

static VMemLocal& vm() {
  static VMemLocal vm;
  return vm;
}

size_t VMemUtil::getAlignment() {
  return vm().alignment;
}

size_t VMemUtil::getPageSize() {
  return vm().pageSize;
}

void* VMemUtil::alloc(size_t length, size_t* allocated, uint32_t flags) {
  size_t msize = IntUtil::alignTo<size_t>(length, vm().pageSize);
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

void VMemUtil::release(void* addr, size_t length) {
  ::munmap(addr, length);
}
#endif // ASMJIT_OS_POSIX

// ============================================================================
// [VMem - Ops]
// ============================================================================

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
  if (c > len) c = len;

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
// [asmjit::host::MemNode]
// ============================================================================

#define M_DIV(x, y) ((x) / (y))
#define M_MOD(x, y) ((x) % (y))

//! \internal
//!
//! Base red-black tree node.
struct RbNode {
  // --------------------------------------------------------------------------
  // [Red-black tree node, key is mem pointer].
  // --------------------------------------------------------------------------

  // Implementation is based on article by Julienne Walker (Public Domain),
  // including C code and original comments. Thanks for the excellent article.

  // Left[0] and right[1] nodes.
  RbNode* node[2];
  // Whether the node is RED.
  uint32_t red;

  // --------------------------------------------------------------------------
  // [Chunk Memory]
  // --------------------------------------------------------------------------

  // Virtual memory address.
  uint8_t* mem;
};

//! \internal
//!
//! Get whether the node is red (NULL or node with red flag).
static ASMJIT_INLINE bool rbIsRed(RbNode* node) {
  return node != NULL && node->red;
}

struct MemNode : public RbNode {
  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  // Get available space.
  ASMJIT_INLINE size_t getAvailable() const { return size - used; }

  ASMJIT_INLINE void fillData(MemNode* other) {
    mem = other->mem;

    size = other->size;
    blocks = other->blocks;
    density = other->density;
    used = other->used;
    largestBlock = other->largestBlock;
    baUsed = other->baUsed;
    baCont = other->baCont;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  MemNode* prev;        // Prev node in list.
  MemNode* next;        // Next node in list.

  size_t size;          // How many bytes contain this node.
  size_t blocks;        // How many blocks are here.
  size_t density;       // Minimum count of allocated bytes in this node (also alignment).
  size_t used;          // How many bytes are used in this node.
  size_t largestBlock;  // Contains largest block that can be allocated.

  size_t* baUsed;       // Contains bits about used blocks       (0 = unused, 1 = used).
  size_t* baCont;       // Contains bits about continuous blocks (0 = stop  , 1 = continue).
};

// ============================================================================
// [asmjit::PermanentNode]
// ============================================================================

//! \internal
//!
//! Permanent node.
struct PermanentNode {
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

  uint8_t* mem;            // Base pointer (virtual memory address).
  size_t size;             // Count of bytes allocated.
  size_t used;             // Count of bytes used.
  PermanentNode* prev;     // Pointer to prev chunk or NULL.
};

// ============================================================================
// [asmjit::VMemPrivate]
// ============================================================================

//! \internal
struct VMemPrivate {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_OS_WINDOWS)
  VMemPrivate();
#else
  VMemPrivate(HANDLE hProcess);
#endif // ASMJIT_OS_WINDOWS
  ~VMemPrivate();

  // --------------------------------------------------------------------------
  // [Allocation]
  // --------------------------------------------------------------------------

  MemNode* createNode(size_t size, size_t density);

  void reset(bool keepVirtualMemory);

  void* allocPermanent(size_t vsize);
  void* allocFreeable(size_t vsize);
  Error release(void* address);
  Error shrink(void* address, size_t used);

  // Helpers to avoid ifdefs in the code.
  ASMJIT_INLINE uint8_t* allocVirtualMemory(size_t size, size_t* vsize) {
    uint32_t flags = kVMemFlagWritable | kVMemFlagExecutable;
#if !defined(ASMJIT_OS_WINDOWS)
    return (uint8_t*)VMemUtil::alloc(size, vsize, flags);
#else
    return (uint8_t*)VMemUtil::allocProcessMemory(_hProcess, size, vsize, flags);
#endif
  }

  ASMJIT_INLINE void freeVirtualMemory(void* vmem, size_t vsize) {
#if !defined(ASMJIT_OS_WINDOWS)
    VMemUtil::release(vmem, vsize);
#else
    VMemUtil::releaseProcessMemory(_hProcess, vmem, vsize);
#endif
  }

  // --------------------------------------------------------------------------
  // [NodeList RB-Tree]
  // --------------------------------------------------------------------------

  bool checkTree();

  void insertNode(MemNode* node);
  MemNode* removeNode(MemNode* node);
  MemNode* findPtr(uint8_t* mem);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_OS_WINDOWS)
  HANDLE _hProcess;            // Process where to allocate memory.
#endif // ASMJIT_OS_WINDOWS
  Lock _lock;                  // Lock for thread safety.

  size_t _newChunkSize;        // Default node size.
  size_t _newChunkDensity;     // Default node density.
  size_t _allocated;           // How many bytes are allocated.
  size_t _used;                // How many bytes are used.

  // Memory nodes list.
  MemNode* _first;
  MemNode* _last;
  MemNode* _optimal;

  // Memory nodes tree.
  MemNode* _root;

  // Permanent memory.
  PermanentNode* _permanent;

  // Whether to keep virtual memory after destroy.
  bool _keepVirtualMemory;
};

// ============================================================================
// [asmjit::VMemPrivate - Construction / Destruction]
// ============================================================================

#if !defined(ASMJIT_OS_WINDOWS)
VMemPrivate::VMemPrivate() :
#else
VMemPrivate::VMemPrivate(HANDLE hProcess) :
  _hProcess(hProcess),
#endif
  _newChunkSize(65536),
  _newChunkDensity(64),
  _allocated(0),
  _used(0),
  _root(NULL),
  _first(NULL),
  _last(NULL),
  _optimal(NULL),
  _permanent(NULL),
  _keepVirtualMemory(false) {}

VMemPrivate::~VMemPrivate() {
  // Freeable memory cleanup - Also frees the virtual memory if configured to.
  reset(_keepVirtualMemory);

  // Permanent memory cleanup - Never frees the virtual memory.
  PermanentNode* node = _permanent;
  while (node) {
    PermanentNode* prev = node->prev;
    ASMJIT_FREE(node);
    node = prev;
  }
}

// ============================================================================
// [asmjit::VMemPrivate - Allocation]
// ============================================================================

// Allocate virtual memory node and MemNode structure.
//
// Returns MemNode* on success, otherwise NULL.
MemNode* VMemPrivate::createNode(size_t size, size_t density) {
  size_t vsize;
  uint8_t* vmem = allocVirtualMemory(size, &vsize);

  // Out of memory.
  if (vmem == NULL) return NULL;

  size_t blocks = (vsize / density);
  size_t bsize = (((blocks + 7) >> 3) + sizeof(size_t) - 1) & ~(size_t)(sizeof(size_t) - 1);

  MemNode* node = static_cast<MemNode*>(ASMJIT_ALLOC(sizeof(MemNode)));
  uint8_t* data = static_cast<uint8_t*>(ASMJIT_ALLOC(bsize * 2));

  // Out of memory.
  if (node == NULL || data == NULL) {
    freeVirtualMemory(vmem, vsize);
    if (node) ASMJIT_FREE(node);
    if (data) ASMJIT_FREE(data);
    return NULL;
  }

  // Initialize RbNode data.
  node->node[0] = NULL;
  node->node[1] = NULL;
  node->red = 1;
  node->mem = vmem;

  // Initialize MemNode data.
  node->prev = NULL;
  node->next = NULL;

  node->size = vsize;
  node->blocks = blocks;
  node->density = density;
  node->used = 0;
  node->largestBlock = vsize;

  ::memset(data, 0, bsize * 2);
  node->baUsed = reinterpret_cast<size_t*>(data);
  node->baCont = reinterpret_cast<size_t*>(data + bsize);

  return node;
}

void VMemPrivate::reset(bool keepVirtualMemory) {
  MemNode* node = _first;

  while (node) {
    MemNode* next = node->next;

    if (!keepVirtualMemory)
      freeVirtualMemory(node->mem, node->size);

    ASMJIT_FREE(node->baUsed);
    ASMJIT_FREE(node);

    node = next;
  }

  _allocated = 0;
  _used = 0;

  _root = NULL;
  _first = NULL;
  _last = NULL;
  _optimal = NULL;
}

void* VMemPrivate::allocPermanent(size_t vsize) {
  static const size_t permanentAlignment = 32;
  static const size_t permanentNodeSize  = 32768;

  size_t over = vsize % permanentAlignment;
  if (over)
    over = permanentAlignment - over;

  size_t alignedSize = vsize + over;
  AutoLock locked(_lock);

  PermanentNode* node = _permanent;

  // Try to find space in allocated chunks.
  while (node && alignedSize > node->getAvailable())
    node = node->prev;

  // Or allocate new node.
  if (node == NULL) {
    size_t nodeSize = permanentNodeSize;

    if (vsize > nodeSize)
      nodeSize = vsize;

    node = static_cast<PermanentNode*>(ASMJIT_ALLOC(sizeof(PermanentNode)));

    // Out of memory.
    if (node == NULL)
      return NULL;

    node->mem = allocVirtualMemory(nodeSize, &node->size);

    // Out of memory.
    if (node->mem == NULL) {
      ASMJIT_FREE(node);
      return NULL;
    }

    node->used = 0;
    node->prev = _permanent;
    _permanent = node;
  }

  // Finally, copy function code to our space we reserved for.
  uint8_t* result = node->mem + node->used;

  // Update Statistics.
  node->used += alignedSize;
  _used += alignedSize;

  // Code can be null to only reserve space for code.
  return static_cast<void*>(result);
}

void* VMemPrivate::allocFreeable(size_t vsize) {
  size_t i;               // Current index.
  size_t need;            // How many we need to be freed.
  size_t minVSize;

  // Align to 32 bytes (our default alignment).
  vsize = (vsize + 31) & ~(size_t)31;
  if (vsize == 0) return NULL;

  AutoLock locked(_lock);
  MemNode* node = _optimal;

  minVSize = _newChunkSize;

  // Try to find memory block in existing nodes.
  while (node) {
    // Skip this node?
    if ((node->getAvailable() < vsize) || (node->largestBlock < vsize && node->largestBlock != 0)) {
      MemNode* next = node->next;
      if (node->getAvailable() < minVSize && node == _optimal && next) _optimal = next;
      node = next;
      continue;
    }

    size_t* up = node->baUsed;    // Current ubits address.
    size_t ubits;                 // Current ubits[0] value.
    size_t bit;                   // Current bit mask.
    size_t blocks = node->blocks; // Count of blocks in node.
    size_t cont = 0;              // How many bits are currently freed in find loop.
    size_t maxCont = 0;           // Largest continuous block (bits count).
    size_t j;

    need = M_DIV((vsize + node->density - 1), node->density);
    i = 0;

    // Try to find node that is large enough.
    while (i < blocks) {
      ubits = *up++;

      // Fast skip used blocks.
      if (ubits == ~(size_t)0) {
        if (cont > maxCont) maxCont = cont;
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
          if (++cont == need) { i += j; i -= cont; goto _Found; }
          continue;
        }

        if (cont > maxCont) maxCont = cont;
        cont = 0;
      }

      i += kBitsPerEntity;
    }

    // Because we traversed entire node, we can set largest node size that
    // will be used to cache next traversing..
    node->largestBlock = maxCont * node->density;

    node = node->next;
  }

  // If we are here, we failed to find existing memory block and we must
  // allocate new.
  {
    size_t chunkSize = _newChunkSize;
    if (chunkSize < vsize) chunkSize = vsize;

    node = createNode(chunkSize, _newChunkDensity);
    if (node == NULL) return NULL;

    // Update binary tree.
    insertNode(node);
    ASMJIT_ASSERT(checkTree());

    // Alloc first node at start.
    i = 0;
    need = (vsize + node->density - 1) / node->density;

    // Update statistics.
    _allocated += node->size;
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
    _used += u;
  }

  // And return pointer to allocated memory.
  uint8_t* result = node->mem + i * node->density;
  ASMJIT_ASSERT(result >= node->mem && result <= node->mem + node->size - vsize);
  return result;
}

Error VMemPrivate::release(void* address) {
  if (address == NULL)
    return kErrorOk;

  AutoLock locked(_lock);

  MemNode* node = findPtr((uint8_t*)address);
  if (node == NULL)
    return kErrorInvalidArgument;

  size_t offset = (size_t)((uint8_t*)address - (uint8_t*)node->mem);
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
  _used -= cont;

  // If page is empty, we can free it.
  if (node->used == 0) {
    // Free memory associated with node (this memory is not accessed
    // anymore so it's safe).
    freeVirtualMemory(node->mem, node->size);
    ASMJIT_FREE(node->baUsed);

    node->baUsed = NULL;
    node->baCont = NULL;

    // Statistics.
    _allocated -= node->size;

    // Remove node. This function can return different node than
    // passed into, but data is copied into previous node if needed.
    ASMJIT_FREE(removeNode(node));
    ASMJIT_ASSERT(checkTree());
  }

  return kErrorOk;
}

Error VMemPrivate::shrink(void* address, size_t used) {
  if (address == NULL)
    return kErrorOk;

  if (used == 0)
    return release(address);

  AutoLock locked(_lock);

  MemNode* node = findPtr((uint8_t*)address);
  if (node == NULL)
    return kErrorInvalidArgument;

  size_t offset = (size_t)((uint8_t*)address - (uint8_t*)node->mem);
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
  _used -= cont;

  return kErrorOk;
}

// ============================================================================
// [asmjit::VMemPrivate - NodeList RB-Tree]
// ============================================================================

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

static ASMJIT_INLINE RbNode* rbRotateSingle(RbNode* root, int dir) {
  RbNode* save = root->node[!dir];

  root->node[!dir] = save->node[dir];
  save->node[dir] = root;

  root->red = 1;
  save->red = 0;

  return save;
}

static ASMJIT_INLINE RbNode* rbRotateDouble(RbNode* root, int dir) {
  root->node[!dir] = rbRotateSingle(root->node[!dir], !dir);
  return rbRotateSingle(root, dir);
}

bool VMemPrivate::checkTree() {
  return rbAssert(_root) > 0;
}

void VMemPrivate::insertNode(MemNode* node) {
  if (_root == NULL) {
    // Empty tree case.
    _root = node;
  }
  else {
    // False tree root.
    RbNode head = { 0 };

    // Grandparent & parent.
    RbNode* g = NULL;
    RbNode* t = &head;

    // Iterator & parent.
    RbNode* p = NULL;
    RbNode* q = t->node[1] = _root;

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
    _root = static_cast<MemNode*>(head.node[1]);
  }

  // Make root black.
  _root->red = 0;

  // Link with others.
  node->prev = _last;

  if (_first == NULL) {
    _first = node;
    _last = node;
    _optimal = node;
  }
  else {
    node->prev = _last;
    _last->next = node;
    _last = node;
  }
}

MemNode* VMemPrivate::removeNode(MemNode* node) {
  // False tree root.
  RbNode head = { 0 };

  // Helpers.
  RbNode* q = &head;
  RbNode* p = NULL;
  RbNode* g = NULL;

  // Found item.
  RbNode* f = NULL;
  int dir = 1;

  // Set up.
  q->node[1] = _root;

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
  _root = static_cast<MemNode*>(head.node[1]);
  if (_root != NULL)
    _root->red = 0;

  // Unlink.
  MemNode* next = static_cast<MemNode*>(q)->next;
  MemNode* prev = static_cast<MemNode*>(q)->prev;

  if (prev)
    prev->next = next;
  else
    _first = next;

  if (next)
    next->prev = prev;
  else
    _last  = prev;

  if (_optimal == q)
    _optimal = prev ? prev : next;

  return static_cast<MemNode*>(q);
}

MemNode* VMemPrivate::findPtr(uint8_t* mem) {
  MemNode* node = _root;

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

// ============================================================================
// [asmjit::VMemMgr - Construction / Destruction]
// ============================================================================

#if !defined(ASMJIT_OS_WINDOWS)
VMemMgr::VMemMgr() {
  VMemPrivate* d = new(std::nothrow) VMemPrivate();
  _d = static_cast<void*>(d);
}
#else
VMemMgr::VMemMgr() {
  HANDLE hProcess = GetCurrentProcess();
  VMemPrivate* d = new(std::nothrow) VMemPrivate(hProcess);
  _d = static_cast<void*>(d);
}

VMemMgr::VMemMgr(HANDLE hProcess) {
  VMemPrivate* d = new(std::nothrow) VMemPrivate(hProcess);
  _d = static_cast<void*>(d);
}
#endif // ASMJIT_OS_WINDOWS

VMemMgr::~VMemMgr() {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);
  delete d;
}

// ============================================================================
// [asmjit::VMemMgr - Reset]
// ============================================================================

void VMemMgr::reset() {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);
  return d->reset(false);
}

// ============================================================================
// [asmjit::VMemMgr - Accessors]
// ============================================================================

#if defined(ASMJIT_OS_WINDOWS)
HANDLE VMemMgr::getProcessHandle() const {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);
  return d->_hProcess;
}
#endif // ASMJIT_OS_WINDOWS

size_t VMemMgr::getUsedBytes() const {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);
  return d->_used;
}

size_t VMemMgr::getAllocatedBytes() const {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);
  return d->_allocated;
}

bool VMemMgr::getKeepVirtualMemory() const {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);
  return d->_keepVirtualMemory;
}

void VMemMgr::setKeepVirtualMemory(bool keepVirtualMemory) {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);
  d->_keepVirtualMemory = keepVirtualMemory;
}

// ============================================================================
// [asmjit::VMemMgr - Alloc / Release]
// ============================================================================

void* VMemMgr::alloc(size_t size, uint32_t type) {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);

  if (type == kVMemAllocPermanent)
    return d->allocPermanent(size);
  else
    return d->allocFreeable(size);
}

Error VMemMgr::release(void* address) {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);
  return d->release(address);
}

Error VMemMgr::shrink(void* address, size_t used) {
  VMemPrivate* d = static_cast<VMemPrivate*>(_d);
  return d->shrink(address, used);
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
