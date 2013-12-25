// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Dependencies]
#include "Build.h"
#include "MemoryManager.h"
#include "Platform.h"

#include <stdio.h>
#include <string.h>

#include <new>

// [Api-Begin]
#include "ApiBegin.h"

// This file contains implementation of virtual memory management for AsmJit
// library. The initial concept is to keep this implementation simple but 
// efficient. There are several goals I decided to write implementation myself.
//
// Goals:
// - We need usually to allocate blocks of 64 bytes long and more.
// - Alignment of allocated blocks is large - 32 bytes or 64 bytes.
// - Keep memory manager information outside allocated virtual memory pages
//   (these pages allows execution of code).
// - Keep implementation small.
//
// I think that implementation is not small and probably not too much readable,
// so there is small know how.
//
// - Implementation is based on bit arrays and binary trees. Bit arrays 
//   contains information about allocated and unused blocks of memory. Each
//   block size describes MemNode::density member. Count of blocks are
//   stored in MemNode::blocks member. For example if density is 64 and 
//   count of blocks is 20, memory node contains 64*20 bytes of memory and
//   smallest possible allocation (and also alignment) is 64 bytes. So density
//   describes also memory alignment. Binary trees are used to enable fast
//   lookup into all addresses allocated by memory manager instance. This is
//   used mainly in MemoryManagerPrivate::free().
//
//   Bit array looks like this (empty = unused, X = used) - Size of block 64
//   -------------------------------------------------------------------------
//   | |X|X| | | | | |X|X|X|X|X|X| | | | | | | | | | | | |X| | | | |X|X|X| | |
//   -------------------------------------------------------------------------
//   Bits array shows that there are 12 allocated blocks of 64 bytes, so total 
//   allocated size is 768 bytes. Maximum count of continuous blocks is 12
//   (see largest gap).

namespace AsmJit {

#pragma warning(disable : 4701)

// ============================================================================
// [Bits Manipulation]
// ============================================================================

#define BITS_PER_ENTITY (sizeof(sysuint_t) * 8)

/*static void _SetBit(sysuint_t* buf, sysuint_t index) ASMJIT_NOTHROW
{
  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  buf += i;
  *buf |= (sysuint_t)1 << j;
}

static void _ClearBit(sysuint_t* buf, sysuint_t index) ASMJIT_NOTHROW
{
  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  buf += i;
  *buf &= ~((sysuint_t)1 << j);
}*/

static void _SetBits(sysuint_t* buf, sysuint_t index, sysuint_t len) ASMJIT_NOTHROW
{
  if (len == 0) return;

  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  // How many bytes process in the first group.
  sysuint_t c = BITS_PER_ENTITY - j;
  if (c > len) c = len;

  // Offset.
  buf += i;

  *buf++ |= (((sysuint_t)-1) >> (BITS_PER_ENTITY - c)) << j;
  len -= c;

  while (len >= BITS_PER_ENTITY)
  {
    *buf++ = (sysuint_t)-1;
    len -= BITS_PER_ENTITY;
  }

  if (len)
  {
    *buf |= (((sysuint_t)-1) >> (BITS_PER_ENTITY - len));
  }
}

/*static void _ClearBits(sysuint_t* buf, sysuint_t index, sysuint_t len) ASMJIT_NOTHROW
{
  if (len == 0) return;

  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  // How many bytes process in the first group.
  sysuint_t c = BITS_PER_ENTITY - j;
  if (c > len) c = len;

  // Offset.
  buf += i;

  *buf++ &= ~((((sysuint_t)-1) >> (BITS_PER_ENTITY - c)) << j);
  len -= c;

  while (len >= BITS_PER_ENTITY)
  {
    *buf++ = 0;
    len -= BITS_PER_ENTITY;
  }

  if (len)
  {
    *buf &= ((sysuint_t)-1) << len;
  }
}
*/
// ============================================================================
// [AsmJit::MemNode]
// ============================================================================

#define M_DIV(x, y) ((x) / (y))
#define M_MOD(x, y) ((x) % (y))

template<typename T>
struct ASMJIT_HIDDEN RbNode
{
  // --------------------------------------------------------------------------
  // [Node red-black tree tree, key is mem pointer].
  // --------------------------------------------------------------------------

  // Implementation is based on article by Julienne Walker (Public Domain),
  // including C code and original comments. Thanks for the excellent article.

  // Left[0] and right[1] nodes.
  T* node[2];
  // Whether the node is RED.
  uint32_t red;

  // --------------------------------------------------------------------------
  // [Chunk Memory]
  // --------------------------------------------------------------------------

  // Virtual memory address.
  uint8_t* mem;
};

// Get whether the node is red (NULL or node with red flag).
template<typename T>
inline bool isRed(RbNode<T>* node)
{
  return node != NULL && node->red;
}

struct ASMJIT_HIDDEN MemNode : public RbNode<MemNode>
{
  // --------------------------------------------------------------------------
  // [Node double-linked list]
  // --------------------------------------------------------------------------

  MemNode* prev;           // Prev node in list.
  MemNode* next;           // Next node in list.

  // --------------------------------------------------------------------------
  // [Chunk Data]
  // --------------------------------------------------------------------------

  sysuint_t size;          // How many bytes contain this node.
  sysuint_t blocks;        // How many blocks are here.
  sysuint_t density;       // Minimum count of allocated bytes in this node (also alignment).
  sysuint_t used;          // How many bytes are used in this node.
  sysuint_t largestBlock;  // Contains largest block that can be allocated.

  sysuint_t* baUsed;       // Contains bits about used blocks.
                           // (0 = unused, 1 = used).
  sysuint_t* baCont;       // Contains bits about continuous blocks.
                           // (0 = stop, 1 = continue).

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  // Get available space.
  inline sysuint_t getAvailable() const ASMJIT_NOTHROW { return size - used; }

  inline void fillData(MemNode* other)
  {
    mem = other->mem;

    size = other->size;
    blocks = other->blocks;
    density = other->density;
    used = other->used;
    largestBlock = other->largestBlock;
    baUsed = other->baUsed;
    baCont = other->baCont;
  }
};

// ============================================================================
// [AsmJit::M_Permanent]
// ============================================================================

//! @brief Permanent node.
struct ASMJIT_HIDDEN PermanentNode
{
  uint8_t* mem;            // Base pointer (virtual memory address).
  sysuint_t size;          // Count of bytes allocated.
  sysuint_t used;          // Count of bytes used.
  PermanentNode* prev;     // Pointer to prev chunk or NULL.

  // Get available space.
  inline sysuint_t getAvailable() const ASMJIT_NOTHROW { return size - used; }
};

// ============================================================================
// [AsmJit::MemoryManagerPrivate]
// ============================================================================

struct ASMJIT_HIDDEN MemoryManagerPrivate
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_WINDOWS)
  MemoryManagerPrivate() ASMJIT_NOTHROW;
#else
  MemoryManagerPrivate(HANDLE hProcess) ASMJIT_NOTHROW;
#endif // ASMJIT_WINDOWS
  ~MemoryManagerPrivate() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Allocation]
  // --------------------------------------------------------------------------

  MemNode* createNode(sysuint_t size, sysuint_t density) ASMJIT_NOTHROW;

  void* allocPermanent(sysuint_t vsize) ASMJIT_NOTHROW;
  void* allocFreeable(sysuint_t vsize) ASMJIT_NOTHROW;

  bool free(void* address) ASMJIT_NOTHROW;
  bool shrink(void* address, sysuint_t used) ASMJIT_NOTHROW;
  void freeAll(bool keepVirtualMemory) ASMJIT_NOTHROW;

  // Helpers to avoid ifdefs in the code.
  inline uint8_t* allocVirtualMemory(sysuint_t size, sysuint_t* vsize) ASMJIT_NOTHROW
  {
#if !defined(ASMJIT_WINDOWS)
    return (uint8_t*)VirtualMemory::alloc(size, vsize, true);
#else
    return (uint8_t*)VirtualMemory::allocProcessMemory(_hProcess, size, vsize, true);
#endif
  }

  inline void freeVirtualMemory(void* vmem, sysuint_t vsize) ASMJIT_NOTHROW
  {
#if !defined(ASMJIT_WINDOWS)
    VirtualMemory::free(vmem, vsize);
#else
    VirtualMemory::freeProcessMemory(_hProcess, vmem, vsize);
#endif
  }

  // --------------------------------------------------------------------------
  // [NodeList RB-Tree]
  // --------------------------------------------------------------------------

  bool checkTree() ASMJIT_NOTHROW;

  void insertNode(MemNode* node) ASMJIT_NOTHROW;
  MemNode* removeNode(MemNode* node) ASMJIT_NOTHROW;
  MemNode* findPtr(uint8_t* mem) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_WINDOWS)
  HANDLE _hProcess;            // Process where to allocate memory.
#endif // ASMJIT_WINDOWS
  Lock _lock;                  // Lock for thread safety.

  sysuint_t _newChunkSize;     // Default node size.
  sysuint_t _newChunkDensity;  // Default node density.
  sysuint_t _allocated;        // How many bytes are allocated.
  sysuint_t _used;             // How many bytes are used.

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
// [AsmJit::MemoryManagerPrivate - Construction / Destruction]
// ============================================================================

#if !defined(ASMJIT_WINDOWS)
MemoryManagerPrivate::MemoryManagerPrivate() ASMJIT_NOTHROW :
#else
MemoryManagerPrivate::MemoryManagerPrivate(HANDLE hProcess) ASMJIT_NOTHROW :
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
  _keepVirtualMemory(false)
{
}

MemoryManagerPrivate::~MemoryManagerPrivate() ASMJIT_NOTHROW
{
  // Freeable memory cleanup - Also frees the virtual memory if configured to.
  freeAll(_keepVirtualMemory);

  // Permanent memory cleanup - Never frees the virtual memory.
  PermanentNode* node = _permanent;
  while (node)
  {
    PermanentNode* prev = node->prev;
    ASMJIT_FREE(node);
    node = prev;
  }
}

// ============================================================================
// [AsmJit::MemoryManagerPrivate - Allocation]
// ============================================================================

// Allocates virtual memory node and MemNode structure.
//
// Returns MemNode* on success, otherwise NULL.
MemNode* MemoryManagerPrivate::createNode(sysuint_t size, sysuint_t density) ASMJIT_NOTHROW
{
  sysuint_t vsize;
  uint8_t* vmem = allocVirtualMemory(size, &vsize);

  // Out of memory.
  if (vmem == NULL) return NULL;

  sysuint_t blocks = (vsize / density);
  sysuint_t bsize = (((blocks + 7) >> 3) + sizeof(sysuint_t) - 1) & ~(sysuint_t)(sizeof(sysuint_t)-1);

  MemNode* node = reinterpret_cast<MemNode*>(ASMJIT_MALLOC(sizeof(MemNode)));
  uint8_t* data = reinterpret_cast<uint8_t*>(ASMJIT_MALLOC(bsize * 2));

  // Out of memory.
  if (node == NULL || data == NULL)
  {
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

  memset(data, 0, bsize * 2);
  node->baUsed = reinterpret_cast<sysuint_t*>(data);
  node->baCont = reinterpret_cast<sysuint_t*>(data + bsize);

  return node;
}

void* MemoryManagerPrivate::allocPermanent(sysuint_t vsize) ASMJIT_NOTHROW
{
  static const sysuint_t permanentAlignment = 32;
  static const sysuint_t permanentNodeSize  = 32768;

  sysuint_t over = vsize % permanentAlignment;
  if (over) over = permanentAlignment - over;
  sysuint_t alignedSize = vsize + over;

  AutoLock locked(_lock);

  PermanentNode* node = _permanent;

  // Try to find space in allocated chunks.
  while (node && alignedSize > node->getAvailable()) node = node->prev;

  // Or allocate new node.
  if (!node)
  {
    sysuint_t nodeSize = permanentNodeSize;
    if (vsize > nodeSize) nodeSize = vsize;

    node = (PermanentNode*)ASMJIT_MALLOC(sizeof(PermanentNode));
    // Out of memory.
    if (node == NULL) return NULL;

    node->mem = allocVirtualMemory(nodeSize, &node->size);
    // Out of memory.
    if (node->mem == NULL) 
    {
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
  return (void*)result;
}

void* MemoryManagerPrivate::allocFreeable(sysuint_t vsize) ASMJIT_NOTHROW
{
  sysuint_t i;               // Current index.
  sysuint_t need;            // How many we need to be freed.
  sysuint_t minVSize;

  // Align to 32 bytes (our default alignment).
  vsize = (vsize + 31) & ~(sysuint_t)31;
  if (vsize == 0) return NULL;

  AutoLock locked(_lock);
  MemNode* node = _optimal;

  minVSize = _newChunkSize;

  // Try to find memory block in existing nodes.
  while (node)
  {
    // Skip this node?
    if ((node->getAvailable() < vsize) || 
        (node->largestBlock < vsize && node->largestBlock != 0))
    {
      MemNode* next = node->next;
      if (node->getAvailable() < minVSize && node == _optimal && next) _optimal = next;
      node = next;
      continue;
    }

    sysuint_t* up = node->baUsed;    // Current ubits address.
    sysuint_t ubits;                 // Current ubits[0] value.
    sysuint_t bit;                   // Current bit mask.
    sysuint_t blocks = node->blocks; // Count of blocks in node.
    sysuint_t cont = 0;              // How many bits are currently freed in find loop.
    sysuint_t maxCont = 0;           // Largest continuous block (bits count).
    sysuint_t j;

    need = M_DIV((vsize + node->density - 1), node->density);
    i = 0;

    // Try to find node that is large enough.
    while (i < blocks)
    {
      ubits = *up++;

      // Fast skip used blocks.
      if (ubits == (sysuint_t)-1)
      { 
        if (cont > maxCont) maxCont = cont;
        cont = 0;

        i += BITS_PER_ENTITY;
        continue;
      }

      sysuint_t max = BITS_PER_ENTITY;
      if (i + max > blocks) max = blocks - i;

      for (j = 0, bit = 1; j < max; bit <<= 1)
      {
        j++;
        if ((ubits & bit) == 0)
        {
          if (++cont == need) { i += j; i -= cont; goto found; }
          continue;
        }

        if (cont > maxCont) maxCont = cont;
        cont = 0;
      }

      i += BITS_PER_ENTITY;
    }

    // Because we traversed entire node, we can set largest node size that
    // will be used to cache next traversing..
    node->largestBlock = maxCont * node->density;

    node = node->next;
  }

  // If we are here, we failed to find existing memory block and we must
  // allocate new.
  {
    sysuint_t chunkSize = _newChunkSize;
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

found:
  // Update bits.
  _SetBits(node->baUsed, i, need);
  _SetBits(node->baCont, i, need - 1);

  // Update statistics.
  {
    sysuint_t u = need * node->density;
    node->used += u;
    node->largestBlock = 0;
    _used += u;
  }

  // And return pointer to allocated memory.
  uint8_t* result = node->mem + i * node->density;
  ASMJIT_ASSERT(result >= node->mem && result <= node->mem + node->size - vsize);
  return result;
}

bool MemoryManagerPrivate::free(void* address) ASMJIT_NOTHROW
{
  if (address == NULL) return true;

  AutoLock locked(_lock);

  MemNode* node = findPtr((uint8_t*)address);
  if (node == NULL)
    return false;

  sysuint_t offset = (sysuint_t)((uint8_t*)address - (uint8_t*)node->mem);
  sysuint_t bitpos = M_DIV(offset, node->density);
  sysuint_t i = (bitpos / BITS_PER_ENTITY);

  sysuint_t* up = node->baUsed + i;  // Current ubits address.
  sysuint_t* cp = node->baCont + i;  // Current cbits address.
  sysuint_t ubits = *up;             // Current ubits[0] value.
  sysuint_t cbits = *cp;             // Current cbits[0] value.
  sysuint_t bit = (sysuint_t)1 << (bitpos % BITS_PER_ENTITY);

  sysuint_t cont = 0;
  bool stop;

  for (;;)
  {
    stop = (cbits & bit) == 0;
    ubits &= ~bit;
    cbits &= ~bit;

    bit <<= 1;
    cont++;

    if (stop || bit == 0)
    {
      *up = ubits;
      *cp = cbits;
      if (stop) break;

      ubits = *++up;
      cbits = *++cp;
      bit = 1;
    }
  }

  // If the freed block is fully allocated node then it's needed to 
  // update 'optimal' pointer in memory manager.
  if (node->used == node->size)
  {
    MemNode* cur = _optimal;

    do {
      cur = cur->prev;
      if (cur == node) { _optimal = node; break; }
    } while (cur);
  }

  // Statistics.
  cont *= node->density;
  if (node->largestBlock < cont) node->largestBlock = cont;
  node->used -= cont;
  _used -= cont;

  // If page is empty, we can free it.
  if (node->used == 0)
  {
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

  return true;
}

bool MemoryManagerPrivate::shrink(void* address, sysuint_t used) ASMJIT_NOTHROW
{
  if (address == NULL) return false;
  if (used == 0) return free(address);

  AutoLock locked(_lock);

  MemNode* node = findPtr((uint8_t*)address);
  if (node == NULL)
    return false;

  sysuint_t offset = (sysuint_t)((uint8_t*)address - (uint8_t*)node->mem);
  sysuint_t bitpos = M_DIV(offset, node->density);
  sysuint_t i = (bitpos / BITS_PER_ENTITY);

  sysuint_t* up = node->baUsed + i;  // Current ubits address.
  sysuint_t* cp = node->baCont + i;  // Current cbits address.
  sysuint_t ubits = *up;             // Current ubits[0] value.
  sysuint_t cbits = *cp;             // Current cbits[0] value.
  sysuint_t bit = (sysuint_t)1 << (bitpos % BITS_PER_ENTITY);

  sysuint_t cont = 0;
  sysuint_t usedBlocks = (used + node->density - 1) / node->density;

  bool stop;

  // Find the first block we can mark as free.
  for (;;)
  {
    stop = (cbits & bit) == 0;
    if (stop) return true;

    if (++cont == usedBlocks) break;

    bit <<= 1;
    if (bit == 0)
    {
      ubits = *++up;
      cbits = *++cp;
      bit = 1;
    }
  }

  // Free the tail blocks.
  cont = (sysuint_t)-1;
  goto enterFreeLoop;

  for (;;)
  {
    stop = (cbits & bit) == 0;
    ubits &= ~bit;
enterFreeLoop:
    cbits &= ~bit;

    bit <<= 1;
    cont++;

    if (stop || bit == 0)
    {
      *up = ubits;
      *cp = cbits;
      if (stop) break;

      ubits = *++up;
      cbits = *++cp;
      bit = 1;
    }
  }

  // Statistics.
  cont *= node->density;
  if (node->largestBlock < cont) node->largestBlock = cont;
  node->used -= cont;
  _used -= cont;

  return true;
}

void MemoryManagerPrivate::freeAll(bool keepVirtualMemory) ASMJIT_NOTHROW
{
  MemNode* node = _first;

  while (node)
  {
    MemNode* next = node->next;
  
    if (!keepVirtualMemory) freeVirtualMemory(node->mem, node->size);
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

// ============================================================================
// [AsmJit::MemoryManagerPrivate - NodeList RB-Tree]
// ============================================================================

static int rbAssert(MemNode* root)
{
  if (root == NULL) return 1;

  MemNode* ln = root->node[0];
  MemNode* rn = root->node[1];

  // Red violation.
  ASMJIT_ASSERT( !(isRed(root) && (isRed(ln) || isRed(rn))) );

  int lh = rbAssert(ln);
  int rh = rbAssert(rn);

  // Invalid btree.
  ASMJIT_ASSERT(ln == NULL || ln->mem < root->mem);
  ASMJIT_ASSERT(rn == NULL || rn->mem > root->mem);

  // Black violation.
  ASMJIT_ASSERT( !(lh != 0 && rh != 0 && lh != rh) );

  // Only count black links.
  if (lh != 0 && rh != 0)
    return isRed(root) ? lh : lh + 1;
  else
    return 0;
}

static inline MemNode* rbRotateSingle(MemNode* root, int dir)
{
  MemNode* save = root->node[!dir];

  root->node[!dir] = save->node[dir];
  save->node[dir] = root;

  root->red = 1;
  save->red = 0;

  return save;
}

static inline MemNode* rbRotateDouble(MemNode* root, int dir)
{
  root->node[!dir] = rbRotateSingle(root->node[!dir], !dir);
  return rbRotateSingle(root, dir);
}

bool MemoryManagerPrivate::checkTree() ASMJIT_NOTHROW
{
  return rbAssert(_root) > 0;
}

void MemoryManagerPrivate::insertNode(MemNode* node) ASMJIT_NOTHROW
{
  if (_root == NULL)
  {
    // Empty tree case.
    _root = node;
  }
  else
  {
    // False tree root.
    RbNode<MemNode> head = {0};

    // Grandparent & parent.
    MemNode* g = NULL;
    MemNode* t = reinterpret_cast<MemNode*>(&head);

    // Iterator & parent.
    MemNode* p = NULL;
    MemNode* q = t->node[1] = _root;

    int dir = 0, last;

    // Search down the tree.
    for (;;)
    {
      if (q == NULL)
      {
        // Insert new node at the bottom.
        q = node;
        p->node[dir] = node;
      }
      else if (isRed(q->node[0]) && isRed(q->node[1]))
      {
        // Color flip.
        q->red = 1;
        q->node[0]->red = 0;
        q->node[1]->red = 0;
      }

      // Fix red violation.
      if (isRed(q) && isRed(p))
      {
        int dir2 = t->node[1] == g;
        t->node[dir2] = (q == p->node[last]) ? rbRotateSingle(g, !last) : rbRotateDouble(g, !last);
      }

      // Stop if found.
      if (q == node) break;

      last = dir;
      dir = q->mem < node->mem;

      // Update helpers.
      if (g != NULL) t = g;
      g = p;
      p = q;
      q = q->node[dir];
    }

    // Update root.
    _root = head.node[1];
  }

  // Make root black.
  _root->red = 0;

  // Link with others.
  node->prev = _last;

  if (_first == NULL)
  {
    _first = node;
    _last = node;
    _optimal = node;
  }
  else
  {
    node->prev = _last;
    _last->next = node;
    _last = node;
  }
}

MemNode* MemoryManagerPrivate::removeNode(MemNode* node) ASMJIT_NOTHROW
{
  // False tree root.
  RbNode<MemNode> head = {0}; 

  // Helpers.
  MemNode* q = reinterpret_cast<MemNode*>(&head);
  MemNode* p = NULL;
  MemNode* g = NULL;
  // Found item.
  MemNode* f = NULL;
  int dir = 1;

  // Set up.
  q->node[1] = _root;

  // Search and push a red down.
  while (q->node[dir] != NULL)
  {
    int last = dir;

    // Update helpers.
    g = p;
    p = q;
    q = q->node[dir];
    dir = q->mem < node->mem;

    // Save found node.
    if (q == node) f = q;

    // Push the red node down.
    if (!isRed(q) && !isRed(q->node[dir]))
    {
      if (isRed(q->node[!dir]))
      {
        p = p->node[last] = rbRotateSingle(q, dir);
      }
      else if (!isRed(q->node[!dir]))
      {
        MemNode* s = p->node[!last];

        if (s != NULL)
        {
          if (!isRed(s->node[!last]) && !isRed(s->node[last]))
          {
            // Color flip.
            p->red = 0;
            s->red = 1;
            q->red = 1;
          }
          else
          {
            int dir2 = g->node[1] == p;

            if (isRed(s->node[last]))
              g->node[dir2] = rbRotateDouble(p, last);
            else if (isRed(s->node[!last]))
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
  ASMJIT_ASSERT(f != reinterpret_cast<MemNode*>(&head));
  ASMJIT_ASSERT(q != reinterpret_cast<MemNode*>(&head));

  if (f != q) f->fillData(q);
  p->node[p->node[1] == q] = q->node[q->node[0] == NULL];

  // Update root and make it black.
  if ((_root = head.node[1]) != NULL) _root->red = 0;

  // Unlink.
  MemNode* next = q->next;
  MemNode* prev = q->prev;

  if (prev) { prev->next = next; } else { _first = next; }
  if (next) { next->prev = prev; } else { _last  = prev; }
  if (_optimal == q) { _optimal = prev ? prev : next; }

  return q;
}

MemNode* MemoryManagerPrivate::findPtr(uint8_t* mem) ASMJIT_NOTHROW
{
  MemNode* cur = _root;
  while (cur)
  {
    uint8_t* curMem = cur->mem;
    if (mem < curMem)
    {
      // Go left.
      cur = cur->node[0];
      continue;
    }
    else
    {
      uint8_t* curEnd = curMem + cur->size;
      if (mem >= curEnd)
      {
        // Go right.
        cur = cur->node[1];
        continue;
      }
      else
      {
        // Match.
        break;
      }
    }
  }
  return cur;
}

// ============================================================================
// [AsmJit::MemoryManager]
// ============================================================================

MemoryManager::MemoryManager() ASMJIT_NOTHROW
{
}

MemoryManager::~MemoryManager() ASMJIT_NOTHROW
{
}

MemoryManager* MemoryManager::getGlobal() ASMJIT_NOTHROW
{
  static VirtualMemoryManager memmgr;
  return &memmgr;
}

// ============================================================================
// [AsmJit::VirtualMemoryManager]
// ============================================================================

#if !defined(ASMJIT_WINDOWS)
VirtualMemoryManager::VirtualMemoryManager() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = new(std::nothrow) MemoryManagerPrivate();
  _d = (void*)d;
}
#else
VirtualMemoryManager::VirtualMemoryManager() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = new(std::nothrow) MemoryManagerPrivate(GetCurrentProcess());
  _d = (void*)d;
}

VirtualMemoryManager::VirtualMemoryManager(HANDLE hProcess) ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = new(std::nothrow) MemoryManagerPrivate(hProcess);
  _d = (void*)d;
}
#endif // ASMJIT_WINDOWS

VirtualMemoryManager::~VirtualMemoryManager() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  delete d;
}

void* VirtualMemoryManager::alloc(sysuint_t size, uint32_t type) ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);

  if (type == MEMORY_ALLOC_PERMANENT) 
    return d->allocPermanent(size);
  else
    return d->allocFreeable(size);
}

bool VirtualMemoryManager::free(void* address) ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  return d->free(address);
}

bool VirtualMemoryManager::shrink(void* address, sysuint_t used) ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  return d->shrink(address, used);
}

void VirtualMemoryManager::freeAll() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);

  // Calling MemoryManager::freeAll() will never keep allocated memory.
  return d->freeAll(false);
}

sysuint_t VirtualMemoryManager::getUsedBytes() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  return d->_used;
}

sysuint_t VirtualMemoryManager::getAllocatedBytes() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  return d->_allocated;
}

bool VirtualMemoryManager::getKeepVirtualMemory() const ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  return d->_keepVirtualMemory;
}

void VirtualMemoryManager::setKeepVirtualMemory(bool keepVirtualMemory) ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  d->_keepVirtualMemory = keepVirtualMemory;
}

// ============================================================================
// [AsmJit::VirtualMemoryManager - Debug]
// ============================================================================

#if defined(ASMJIT_MEMORY_MANAGER_DUMP)

struct ASMJIT_HIDDEN GraphVizContext
{
  GraphVizContext();
  ~GraphVizContext();

  bool openFile(const char* fileName);
  void closeFile();

  void dumpNode(MemNode* node);
  void connect(MemNode* node, MemNode* other, const char* dst);

  FILE* file;
};

GraphVizContext::GraphVizContext() :
  file(NULL)
{
}

GraphVizContext::~GraphVizContext()
{
  closeFile();
}

bool GraphVizContext::openFile(const char* fileName)
{
  file = fopen(fileName, "w");
  return file != NULL;
}

void GraphVizContext::closeFile()
{
  if (file) { fclose(file); file = NULL; }
}

void GraphVizContext::dumpNode(MemNode* node)
{
  fprintf(file, "  NODE_%p [shape=record, style=filled, color=%s, label=\"<L>|<C>Mem: %p, Used: %d/%d|<R>\"];\n",
    node,
    node->red ? "red" : "gray",
    node->mem, node->used, node->size);

  if (node->node[0]) connect(node, node->node[0], "L");
  if (node->node[1]) connect(node, node->node[1], "R");
}

void GraphVizContext::connect(MemNode* node, MemNode* other, const char* dst)
{
  dumpNode(other);

  fprintf(file, "  NODE_%p:%s -> NODE_%p:C", node, dst, other);
  if (other->red) fprintf(file, " [style=bold, color=red]");
  fprintf(file, ";\n");
}

void VirtualMemoryManager::dump(const char* fileName)
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  GraphVizContext ctx;
  if (!ctx.openFile(fileName)) return;

  fprintf(ctx.file, "digraph {\n");
  if (d->_root) ctx.dumpNode(d->_root);
  fprintf(ctx.file, "}\n");
}
#endif // ASMJIT_MEMORY_MANAGER_DUMP

#pragma warning(default : 4505 4701)

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"
