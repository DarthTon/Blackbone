#include "Remap.h"
#include "BlackBoneDrv.h"
#include "Routines.h"

#pragma alloc_text(PAGE, BBProcessNotify)


/// <summary>
/// Process termination handler
/// </summary>
/// <param name="ParentId">Parent PID</param>
/// <param name="ProcessId">PID</param>
/// <param name="Create">TRUE if process was created</param>
VOID BBProcessNotify( IN HANDLE ParentId, IN HANDLE ProcessId, IN BOOLEAN Create )
{
    UNREFERENCED_PARAMETER( ParentId );
    PPROCESS_MAP_ENTRY pProcessEntry = NULL;
    PMEM_PHYS_PROCESS_ENTRY pPhysProcessEntry = NULL;

    if (Create == FALSE)
    {
        pPhysProcessEntry = BBLookupPhysProcessEntry( ProcessId );
        if (pPhysProcessEntry != NULL)
        {
            DPRINT( "BlackBone: %s: Target process %u shutdown. Physical memory Cleanup\n", __FUNCTION__, ProcessId );
            BBCleanupProcessPhysEntry( pPhysProcessEntry, TRUE );
        }

        KeAcquireGuardedMutex( &g_globalLock );

        pProcessEntry = BBLookupProcessEntry( ProcessId, FALSE );

        // Target process shutdown
        if (pProcessEntry != NULL)
        {
            DPRINT( "BlackBone: %s: Target process %u shutdown. Cleanup\n", __FUNCTION__, pProcessEntry->target.pid );
            BBCleanupProcessEntry( pProcessEntry );
        }
        else
        {
            pProcessEntry = BBLookupProcessEntry( ProcessId, TRUE );

            // Host process shutdown
            if (pProcessEntry != NULL)
                BBCleanupHostProcess( pProcessEntry );
        }

        KeReleaseGuardedMutex( &g_globalLock );
    }
}