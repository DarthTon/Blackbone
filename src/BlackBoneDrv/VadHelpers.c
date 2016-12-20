#include "Private.h"
#include "VadHelpers.h"

#pragma alloc_text(PAGE, MiPromoteNode)
#pragma alloc_text(PAGE, MiRebalanceNode)
#pragma alloc_text(PAGE, MiRemoveNode)
#pragma alloc_text(PAGE, MiFindNodeOrParent)

VOID
MiPromoteNode(
    IN PMMADDRESS_NODE C
    )

/*++

    Routine Description:

        This routine performs the fundamental adjustment required for balancing
        the binary tree during insert and delete operations.  Simply put, the
        designated node is promoted in such a way that it rises one level in
        the tree and its parent drops one level in the tree, becoming now the
        child of the designated node.  Generally the path length to the subtree
        "opposite" the original parent.  Balancing occurs as the caller chooses
        which nodes to promote according to the balanced tree algorithms from
        Knuth.

        This is not the same as a splay operation, typically a splay "promotes"
        a designated node twice.

        Note that the pointer to the root node of the tree is assumed to be
        contained in a MMADDRESS_NODE structure itself, to allow the
        algorithms below to change the root of the tree without checking
        for special cases.  Note also that this is an internal routine,
        and the caller guarantees that it never requests to promote the
        root itself.

        This routine only updates the tree links; the caller must update
        the balance factors as appropriate.

    Arguments:

        C - pointer to the child node to be promoted in the tree.

    Return Value:

        None.

--*/

{
    PMMADDRESS_NODE P;
    PMMADDRESS_NODE G;

    //
    // Capture the current parent and grandparent (may be the root).
    //

    P = SANITIZE_PARENT_NODE( C->u1.Parent );
    G = SANITIZE_PARENT_NODE( P->u1.Parent );

    //
    // Break down the promotion into two cases based upon whether C
    // is a left or right child.
    //

    if (P->LeftChild == C) {

        //
        // This promotion looks like this:
        //
        //          G           G
        //          |           |
        //          P           C
        //         / \   =>    / \
                                                //        C   z       x   P
        //       / \             / \
                                                //      x   y           y   z
        //

        P->LeftChild = C->RightChild;

        if (P->LeftChild != NULL) {

            P->LeftChild->u1.Parent = MI_MAKE_PARENT( P, P->LeftChild->u1.Balance );
        }

        C->RightChild = P;

        //
        // Fall through to update parent and G <-> C relationship in
        // common code.
        //

    }
    else {

        //
        // This promotion looks like this:
        //
        //        G               G
        //        |               |
        //        P               C
        //       / \     =>      / \
                                                //      x   C           P   z
        //         / \         / \
                                                //        y   z       x   y
        //

        P->RightChild = C->LeftChild;

        if (P->RightChild != NULL) {
            P->RightChild->u1.Parent = MI_MAKE_PARENT( P, P->RightChild->u1.Balance );
        }

        C->LeftChild = P;
    }

    //
    // Update parent of P, for either case above.
    //

    P->u1.Parent = MI_MAKE_PARENT( C, P->u1.Balance );

    //
    // Finally update G <-> C links for either case above.
    //

    if (G->LeftChild == P) {
        G->LeftChild = C;
    }
    else {
        G->RightChild = C;
    }
    C->u1.Parent = MI_MAKE_PARENT( G, C->u1.Balance );
}

ULONG
MiRebalanceNode(
    IN PMMADDRESS_NODE S
    )

/*++

    Routine Description:

        This routine performs a rebalance around the input node S, for which the
        Balance factor has just effectively become +2 or -2.  When called, the
        Balance factor still has a value of +1 or -1, but the respective longer
        side has just become one longer as the result of an insert or delete
        operation.

        This routine effectively implements steps A7.iii (test for Case 1 or
        Case 2) and steps A8 and A9 of Knuth's balanced insertion algorithm,
        plus it handles Case 3 identified in the delete section, which can
        only happen on deletes.

        The trick is, to convince yourself that while traveling from the
        insertion point at the bottom of the tree up, that there are only
        these two cases, and that when traveling up from the deletion point,
        that there are just these three cases.  Knuth says it is obvious!

    Arguments:

        S - pointer to the node which has just become unbalanced.

    Return Value:

        TRUE if Case 3 was detected (causes delete algorithm to terminate).

    Environment:

        Kernel mode.  The PFN lock is held for some of the tables.

--*/

{
    PMMADDRESS_NODE R, P;
    SCHAR a;

    //
    // Capture which side is unbalanced.
    //

    a = (SCHAR)S->u1.Balance;

    if (a == +1) {
        R = S->RightChild;
    }
    else {
        R = S->LeftChild;
    }

    //
    // If the balance of R and S are the same (Case 1 in Knuth) then a single
    // promotion of R will do the single rotation.  (Step A8, A10)
    //
    // Here is a diagram of the Case 1 transformation, for a == +1 (a mirror
    // image transformation occurs when a == -1), and where the subtree
    // heights are h and h+1 as shown (++ indicates the node out of balance):
    //
    //                  |                   |
    //                  S++                 R
    //                 / \                 / \
                        //               (h)  R+     ==>      S  (h+1)
    //                   / \             / \
                        //                 (h) (h+1)       (h) (h)
    //
    // Note that on an insert we can hit this case by inserting an item in the
    // right subtree of R.  The original height of the subtree before the insert
    // was h+2, and it is still h+2 after the rebalance, so insert rebalancing
    // may terminate.
    //
    // On a delete we can hit this case by deleting a node from the left subtree
    // of S.  The height of the subtree before the delete was h+3, and after the
    // rebalance it is h+2, so rebalancing must continue up the tree.
    //

    if ((SCHAR)R->u1.Balance == a) {

        MiPromoteNode( R );
        R->u1.Balance = 0;
        S->u1.Balance = 0;

        return FALSE;
    }

    //
    // Otherwise, we have to promote the appropriate child of R twice (Case 2
    // in Knuth).  (Step A9, A10)
    //
    // Here is a diagram of the Case 2 transformation, for a == +1 (a mirror
    // image transformation occurs when a == -1), and where the subtree
    // heights are h and h-1 as shown.  There are actually two minor subcases,
    // differing only in the original balance of P (++ indicates the node out
    // of balance).
    //
    //                  |                   |
    //                  S++                 P
    //                 / \                 / \
                        //                /   \               /   \
                        //               /     \             /     \
                        //             (h)      R-   ==>    S-      R
    //                     / \         / \     / \
                        //                    P+ (h)     (h)(h-1)(h) (h)
    //                   / \
                        //               (h-1) (h)
    //
    //
    //                  |                   |
    //                  S++                 P
    //                 / \                 / \
                        //                /   \               /   \
                        //               /     \             /     \
                        //             (h)      R-   ==>    S       R+
    //                     / \         / \     / \
                        //                    P- (h)     (h) (h)(h-1)(h)
    //                   / \
                        //                 (h) (h-1)
    //
    // Note that on an insert we can hit this case by inserting an item in the
    // left subtree of R.  The original height of the subtree before the insert
    // was h+2, and it is still h+2 after the rebalance, so insert rebalancing
    // may terminate.
    //
    // On a delete we can hit this case by deleting a node from the left subtree
    // of S.  The height of the subtree before the delete was h+3, and after the
    // rebalance it is h+2, so rebalancing must continue up the tree.
    //

    if ((SCHAR)R->u1.Balance == -a) {

        //
        // Pick up the appropriate child P for the double rotation (Link(-a,R)).
        //

        if (a == 1) {
            P = R->LeftChild;
        }
        else {
            P = R->RightChild;
        }

        //
        // Promote him twice to implement the double rotation.
        //

        MiPromoteNode( P );
        MiPromoteNode( P );

        //
        // Now adjust the balance factors.
        //

        S->u1.Balance = 0;
        R->u1.Balance = 0;
        if ((SCHAR)P->u1.Balance == a) {
            COUNT_BALANCE_MAX( (SCHAR)-a );
            S->u1.Balance = (ULONG_PTR)-a;
        }
        else if ((SCHAR)P->u1.Balance == -a) {
            COUNT_BALANCE_MAX( (SCHAR)a );
            R->u1.Balance = (ULONG_PTR)a;
        }

        P->u1.Balance = 0;
        return FALSE;
    }

    //
    // Otherwise this is Case 3 which can only happen on Delete (identical
    // to Case 1 except R->u1.Balance == 0).  We do a single rotation, adjust
    // the balance factors appropriately, and return TRUE.  Note that the
    // balance of S stays the same.
    //
    // Here is a diagram of the Case 3 transformation, for a == +1 (a mirror
    // image transformation occurs when a == -1), and where the subtree
    // heights are h and h+1 as shown (++ indicates the node out of balance):
    //
    //                  |                   |
    //                  S++                 R-
    //                 / \                 / \
                        //               (h)  R      ==>      S+ (h+1)
    //                   / \             / \
                        //                (h+1)(h+1)       (h) (h+1)
    //
    // This case can not occur on an insert, because it is impossible for
    // a single insert to balance R, yet somehow grow the right subtree of
    // S at the same time.  As we move up the tree adjusting balance factors
    // after an insert, we terminate the algorithm if a node becomes balanced,
    // because that means the subtree length did not change!
    //
    // On a delete we can hit this case by deleting a node from the left
    // subtree of S.  The height of the subtree before the delete was h+3,
    // and after the rebalance it is still h+3, so rebalancing may terminate
    // in the delete path.
    //

    MiPromoteNode( R );
    COUNT_BALANCE_MAX( (SCHAR)-a );
    R->u1.Balance = -a;
    return TRUE;
}


/*
VOID
MiInsertNode (
IN PMMADDRESS_NODE NodeToInsert,
IN PMM_AVL_TABLE Table
)

/ *++

Routine Description:

This function inserts a new element in a table.

Arguments:

NodeToInsert - The initialized address node to insert.

Table - Pointer to the table in which to insert the new node.

Return Value:

None.

Environment:

Kernel mode.  The PFN lock is held for some of the tables.

--* /

{
    //
    // Holds a pointer to the node in the table or what would be the
    // parent of the node.
    //

    PMMADDRESS_NODE NodeOrParent;
    TABLE_SEARCH_RESULT SearchResult;

    SearchResult = MiFindNodeOrParent( Table,
                                       (ULONG_PTR)((PMMVAD_SHORT)NodeToInsert)->StartingVpn,
                                       &NodeOrParent );


    //
    // The node wasn't in the (possibly empty) tree.
    //
    // We just check that the table isn't getting too big.
    //

    NodeToInsert->LeftChild = NULL;
    NodeToInsert->RightChild = NULL;

    Table->NumberGenericTableElements += 1;

    //
    // Insert the new node in the tree.
    //

    if (SearchResult == TableEmptyTree) {

        Table->BalancedRoot.RightChild = NodeToInsert;
        NodeToInsert->u1.Parent = &Table->BalancedRoot;
        Table->DepthOfTree = 1;

    }
    else {

        PMMADDRESS_NODE R = NodeToInsert;
        PMMADDRESS_NODE S = NodeOrParent;

        if (SearchResult == TableInsertAsLeft) {
            NodeOrParent->LeftChild = NodeToInsert;
        }
        else {
            NodeOrParent->RightChild = NodeToInsert;
        }

        NodeToInsert->u1.Parent = NodeOrParent;

        //
        // The above completes the standard binary tree insertion, which
        // happens to correspond to steps A1-A5 of Knuth's "balanced tree
        // search and insertion" algorithm.  Now comes the time to adjust
        // balance factors and possibly do a single or double rotation as
        // in steps A6-A10.
        //
        // Set the Balance factor in the root to a convenient value
        // to simplify loop control.
        //

        COUNT_BALANCE_MAX( (SCHAR)-1 );
        Table->BalancedRoot.u1.Balance = (ULONG_PTR)-1;

        //
        // Now loop to adjust balance factors and see if any balance operations
        // must be performed, using NodeOrParent to ascend the tree.
        //

        for (;;) {

            SCHAR a;

            //
            // Calculate the next adjustment.
            //

            a = 1;
            if (MiIsLeftChild( R )) {
                a = -1;
            }

            //
            // If this node was balanced, show that it is no longer and
            // keep looping.  This is essentially A6 of Knuth's algorithm,
            // where he updates all of the intermediate nodes on the
            // insertion path which previously had balance factors of 0.
            // We are looping up the tree via Parent pointers rather than
            // down the tree as in Knuth.
            //

            if (S->u1.Balance == 0) {

                COUNT_BALANCE_MAX( (SCHAR)a );
                S->u1.Balance = a;
                R = S;
                S = SANITIZE_PARENT_NODE( S->u1.Parent );
            }
            else if ((SCHAR)S->u1.Balance != a) {


                //
                // If this node has the opposite balance, then the tree got
                // more balanced (or we hit the root) and we are done.
                //
                // Step A7.ii
                //

                S->u1.Balance = 0;

                //
                // If S is actually the root, then this means the depth
                // of the tree just increased by 1!  (This is essentially
                // A7.i, but we just initialized the root balance to force
                // it through here.)
                //

                if (Table->BalancedRoot.u1.Balance == 0) {
                    Table->DepthOfTree += 1;
                }

                break;
            }
            else {

                //
                // The tree became unbalanced (path length differs
                // by 2 below us) and we need to do one of the balancing
                // operations, and then we are done.  The RebalanceNode routine
                // does steps A7.iii, A8 and A9.
                //

                MiRebalanceNode( S );
                break;
            }
        }
    }

    //
    // Sanity check tree size and depth.
    //


    return;
}*/

VOID
MiRemoveNode(
    IN PMMADDRESS_NODE NodeToDelete,
    IN PMM_AVL_TABLE Table
    )

/*++

    Routine Description:

        This routine deletes the specified node from the balanced tree, rebalancing
        as necessary.  If the NodeToDelete has at least one NULL child pointers,
        then it is chosen as the EasyDelete, otherwise a subtree predecessor or
        successor is found as the EasyDelete.  In either case the EasyDelete is
        deleted and the tree is rebalanced.  Finally if the NodeToDelete was
        different than the EasyDelete, then the EasyDelete is linked back into the
        tree in place of the NodeToDelete.

    Arguments:

    NodeToDelete - Pointer to the node which the caller wishes to delete.

    Table - The generic table in which the delete is to occur.

    Return Value:

        None.

    Environment:

        Kernel mode.  The PFN lock is held for some of the tables.

--*/

{
    PMMADDRESS_NODE Parent;
    PMMADDRESS_NODE EasyDelete;
    PMMADDRESS_NODE P;
    SCHAR a;

    //
    // If the NodeToDelete has at least one NULL child pointer, then we can
    // delete it directly.
    //

    if ((NodeToDelete->LeftChild == NULL) ||
         (NodeToDelete->RightChild == NULL)) {

        EasyDelete = NodeToDelete;
    }

    //
    // Otherwise, we may as well pick the longest side to delete from (if one is
    // is longer), as that reduces the probability that we will have to
    // rebalance.
    //

    else if ((SCHAR)NodeToDelete->u1.Balance >= 0) {

        //
        // Pick up the subtree successor.
        //

        EasyDelete = NodeToDelete->RightChild;
        while (EasyDelete->LeftChild != NULL) {
            EasyDelete = EasyDelete->LeftChild;
        }
    }
    else {

        //
        // Pick up the subtree predecessor.
        //

        EasyDelete = NodeToDelete->LeftChild;
        while (EasyDelete->RightChild != NULL) {
            EasyDelete = EasyDelete->RightChild;
        }
    }

    //
    // Rebalancing must know which side of the first parent the delete occurred
    // on.  Assume it is the left side and otherwise correct below.
    //

    a = -1;

    //
    // Now we can do the simple deletion for the no left child case.
    //

    if (EasyDelete->LeftChild == NULL) {

        Parent = SANITIZE_PARENT_NODE( EasyDelete->u1.Parent );

        if (MiIsLeftChild( EasyDelete )) {
            Parent->LeftChild = EasyDelete->RightChild;
        }
        else {
            Parent->RightChild = EasyDelete->RightChild;
            a = 1;
        }

        if (EasyDelete->RightChild != NULL) {
            EasyDelete->RightChild->u1.Parent = MI_MAKE_PARENT( Parent, EasyDelete->RightChild->u1.Balance );
        }

        //
        // Now we can do the simple deletion for the no right child case,
        // plus we know there is a left child.
        //

    }
    else {

        Parent = SANITIZE_PARENT_NODE( EasyDelete->u1.Parent );

        if (MiIsLeftChild( EasyDelete )) {
            Parent->LeftChild = EasyDelete->LeftChild;
        }
        else {
            Parent->RightChild = EasyDelete->LeftChild;
            a = 1;
        }

        EasyDelete->LeftChild->u1.Parent = MI_MAKE_PARENT( Parent,
                                                           EasyDelete->LeftChild->u1.Balance );
    }

    //
    // For delete rebalancing, set the balance at the root to 0 to properly
    // terminate the rebalance without special tests, and to be able to detect
    // if the depth of the tree actually decreased.
    //

#if defined( _WIN81_ ) || defined ( _WIN10_ )
    Table->BalancedRoot->u1.Balance = 0;
#else
    Table->BalancedRoot.u1.Balance = 0;
#endif
    P = SANITIZE_PARENT_NODE( EasyDelete->u1.Parent );

    //
    // Loop until the tree is balanced.
    //

    for (;;) {

        //
        // First handle the case where the tree became more balanced.  Zero
        // the balance factor, calculate a for the next loop and move on to
        // the parent.
        //

        if ((SCHAR)P->u1.Balance == a) {

            P->u1.Balance = 0;

            //
            // If this node is curently balanced, we can show it is now unbalanced
            // and terminate the scan since the subtree length has not changed.
            // (This may be the root, since we set Balance to 0 above!)
            //

        }
        else if (P->u1.Balance == 0)
        {
            COUNT_BALANCE_MAX( (SCHAR)-a );
            P->u1.Balance = -a;

            //
            // If we shortened the depth all the way back to the root, then
            // the tree really has one less level.
            //

#if !defined( _WIN81_ ) && !defined ( _WIN10_ )
            if (Table->BalancedRoot.u1.Balance != 0) {
                Table->DepthOfTree -= 1;
            }
#endif

            break;

            //
            // Otherwise we made the short side 2 levels less than the long side,
            // and rebalancing is required.  On return, some node has been promoted
            // to above node P.  If Case 3 from Knuth was not encountered, then we
            // want to effectively resume rebalancing from P's original parent which
            // is effectively its grandparent now.
            //

        }
        else
        {

            //
            // We are done if Case 3 was hit, i.e., the depth of this subtree is
            // now the same as before the delete.
            //

            if (MiRebalanceNode( P ))
            {
                break;
            }

            P = SANITIZE_PARENT_NODE( P->u1.Parent );
        }

        a = -1;
        if (MiIsRightChild( P ))
        {
            a = 1;
        }

        P = SANITIZE_PARENT_NODE( P->u1.Parent );
    }

    //
    // Finally, if we actually deleted a predecessor/successor of the
    // NodeToDelete, we will link him back into the tree to replace
    // NodeToDelete before returning.  Note that NodeToDelete did have
    // both child links filled in, but that may no longer be the case
    // at this point.
    //

    if (NodeToDelete != EasyDelete)
    {

        //
        // Note carefully - VADs are of differing sizes therefore it is not safe
        // to just overlay the EasyDelete node with the NodeToDelete like the
        // rtl avl code does.
        //
        // Copy just the links, preserving the rest of the original EasyDelete
        // VAD.
        //

        EasyDelete->u1.Parent = NodeToDelete->u1.Parent;
        EasyDelete->LeftChild = NodeToDelete->LeftChild;
        EasyDelete->RightChild = NodeToDelete->RightChild;

        if (MiIsLeftChild( NodeToDelete ))
        {
            Parent = SANITIZE_PARENT_NODE( EasyDelete->u1.Parent );
            Parent->LeftChild = EasyDelete;
        }
        else
        {
            Parent = SANITIZE_PARENT_NODE( EasyDelete->u1.Parent );
            Parent->RightChild = EasyDelete;
        }
        if (EasyDelete->LeftChild != NULL)
        {
            EasyDelete->LeftChild->u1.Parent = MI_MAKE_PARENT( EasyDelete,
                                                               EasyDelete->LeftChild->u1.Balance );
        }
        if (EasyDelete->RightChild != NULL)
        {
            EasyDelete->RightChild->u1.Parent = MI_MAKE_PARENT( EasyDelete,
                                                                EasyDelete->RightChild->u1.Balance );
        }
    }

    return;
}

TABLE_SEARCH_RESULT
MiFindNodeOrParent(
    IN PMM_AVL_TABLE Table,
    IN ULONG_PTR StartingVpn,
    OUT PMMADDRESS_NODE *NodeOrParent
    )

/*++

    Routine Description:

        This routine is used by all of the routines of the generic
        table package to locate the a node in the tree.  It will
        find and return (via the NodeOrParent parameter) the node
        with the given key, or if that node is not in the tree it
        will return (via the NodeOrParent parameter) a pointer to
        the parent.

    Arguments:

        Table - The generic table to search for the key.

        StartingVpn - The starting virtual page number.

        NodeOrParent - Will be set to point to the node containing the
        the key or what should be the parent of the node
        if it were in the tree.  Note that this will *NOT*
        be set if the search result is TableEmptyTree.

    Return Value:

        TABLE_SEARCH_RESULT - TableEmptyTree: The tree was empty.  NodeOrParent
        is *not* altered.

        TableFoundNode: A node with the key is in the tree.
        NodeOrParent points to that node.

        TableInsertAsLeft: Node with key was not found.
        NodeOrParent points to what would
        be parent.  The node would be the
        left child.

        TableInsertAsRight: Node with key was not found.
        NodeOrParent points to what would
        be parent.  The node would be
        the right child.

    Environment:

        Kernel mode.  The PFN lock is held for some of the tables.

--*/

{
    PMMADDRESS_NODE Child;
    PMMADDRESS_NODE NodeToExamine;
    PMMVAD_SHORT    VpnCompare;
    ULONG_PTR       startVpn;
    ULONG_PTR       endVpn;

    if (Table->NumberGenericTableElements == 0) {
        return TableEmptyTree;
    }

    NodeToExamine = (PMMADDRESS_NODE)GET_VAD_ROOT( Table );

    for (;;) {

        VpnCompare = (PMMVAD_SHORT)NodeToExamine;
        startVpn = VpnCompare->StartingVpn;
        endVpn = VpnCompare->EndingVpn;

#if defined( _WIN81_ ) || defined( _WIN10_ )
        startVpn |= (ULONG_PTR)VpnCompare->StartingVpnHigh << 32;
        endVpn |= (ULONG_PTR)VpnCompare->EndingVpnHigh << 32;
#endif  

        //
        // Compare the buffer with the key in the tree element.
        //

        if (StartingVpn < startVpn) {

            Child = NodeToExamine->LeftChild;

            if (Child != NULL) {
                NodeToExamine = Child;
            }
            else {

                //
                // Node is not in the tree.  Set the output
                // parameter to point to what would be its
                // parent and return which child it would be.
                //

                *NodeOrParent = NodeToExamine;
                return TableInsertAsLeft;
            }
        }
        else if (StartingVpn <= endVpn) {

            //
            // This is the node.
            //

            *NodeOrParent = NodeToExamine;
            return TableFoundNode;
        }
        else {

            Child = NodeToExamine->RightChild;

            if (Child != NULL) {
                NodeToExamine = Child;
            }
            else {

                //
                // Node is not in the tree.  Set the output
                // parameter to point to what would be its
                // parent and return which child it would be.
                //

                *NodeOrParent = NodeToExamine;
                return TableInsertAsRight;
            }
        }

    };
}