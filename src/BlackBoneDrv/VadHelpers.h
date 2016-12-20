#pragma once

#define SANITIZE_PARENT_NODE(Parent) ((PMMADDRESS_NODE)(((ULONG_PTR)(Parent)) & ~0x3))

//
// Various Rtl macros that reference Parent use private versions here since
// Parent is overloaded with Balance.
//

//
//  The macro function Parent takes as input a pointer to a splay link in a
//  tree and returns a pointer to the splay link of the parent of the input
//  node.  If the input node is the root of the tree the return value is
//  equal to the input value.
//
//  PRTL_SPLAY_LINKS
//  MiParent (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define MiParent(Links) (               \
    (PRTL_SPLAY_LINKS)(SANITIZE_PARENT_NODE((Links)->u1.Parent)) \
    )

//
//  The macro function IsLeftChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the left child of its
//  parent, otherwise it returns FALSE.
//
//  BOOLEAN
//  MiIsLeftChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define MiIsLeftChild(Links) (                                   \
    (RtlLeftChild(MiParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )

//
//  The macro function IsRightChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the right child of its
//  parent, otherwise it returns FALSE.
//
//  BOOLEAN
//  MiIsRightChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define MiIsRightChild(Links) (                                   \
    (RtlRightChild(MiParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )

#define MI_MAKE_PARENT(ParentNode, ExistingBalance) \
    (PMMADDRESS_NODE)((ULONG_PTR)(ParentNode) | (((ULONG_PTR)ExistingBalance) & 0x3))

#define COUNT_BALANCE_MAX(a)


TABLE_SEARCH_RESULT MiFindNodeOrParent( IN PMM_AVL_TABLE Table, IN ULONG_PTR StartingVpn, OUT PMMADDRESS_NODE *NodeOrParent );
VOID MiPromoteNode( IN PMMADDRESS_NODE C );
ULONG MiRebalanceNode( IN PMMADDRESS_NODE S );
VOID MiRemoveNode( IN PMMADDRESS_NODE NodeToDelete, IN PMM_AVL_TABLE Table );