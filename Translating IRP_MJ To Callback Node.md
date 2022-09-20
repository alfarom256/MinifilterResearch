```C++
__int64 __fastcall FltpSetCallbacksForInstance(
        __int64 lpPoolWithTag,
        __int64 lpPoolWithTagOffset0x230,
        int dwNumCallbackNodes)
{
  __int64 lpFltFilter; // r15
  __int64 v6; // rbx
  _FLT_OPERATION_REGISTRATION *lpFilterOperations; // r14
  __int64 v9; // rbx
  unsigned __int8 v10; // cl
  _QWORD *v11; // r9
  __int64 v12; // rax
  __int64 v13; // rcx
  __int64 v14; // rbx
  unsigned int v15; // edi
  _DWORD *v16; // rbp
  ERESOURCE *v17; // rbx

  
  lpFltFilter = *(_QWORD *)(lpPoolWithTag + 0x38);
  v6 = qword_1C002A7A0;
  lpFilterOperations = *(_FLT_OPERATION_REGISTRATION **)(*(_QWORD *)(lpPoolWithTag + 64) + 0x1A8i64);
  KeEnterGuardedRegion();
  v9 = ExAcquireCacheAwarePushLockSharedEx(v6, 0i64);
  while ( lpFilterOperations->MajorFunction != 0x80 && dwNumCallbackNodes )
  {
    if ( (unsigned __int8)(lpFilterOperations->MajorFunction + 20) > 1u
      && (lpFilterOperations->PreOperation || lpFilterOperations->PostOperation) )
    {
    //
    // The callback node index is equal to
    // the major fn + 22
    //
    // Now we can skip finding 
    // each instance's callback nodes
    // 
    // Unless otherwise changed, callback
    // nodes appear to be consistent 
    // and equal across all instances
    //
      v10 = lpFilterOperations->MajorFunction + 22;
      if ( v10 < 0x32u )
      {
        if ( *(_QWORD *)(lpPoolWithTag + 8i64 * v10 + 160) )
        {
          ExReleaseCacheAwarePushLockSharedEx(v9, 0i64);
          KeLeaveGuardedRegion();
          return 3223060493i64;
        }
        FltpInitializeCallbackNode(
	      // the callback node 
          (unsigned int)lpPoolWithTagOffset0x230,
          (_FLT_OPERATION_REGISTRATION *)(_DWORD)lpFilterOperations,
          0i64,
          0i64,
          0i64,
          0i64,
          lpPoolWithTag,
          v10);
        lpPoolWithTagOffset0x230 += 48i64;      // called for demominifilter
                                                // 
        --dwNumCallbackNodes;
      }
    }
    ++lpFilterOperations;
  }
  v11 = *(_QWORD **)(lpPoolWithTag + 64);
  v12 = v11[47];
  if ( v12 && dwNumCallbackNodes )
  {
    *(_DWORD *)(lpPoolWithTagOffset0x230 + 40) = 0;
    *(_QWORD *)(lpPoolWithTagOffset0x230 + 16) = lpPoolWithTag;
    *(_QWORD *)(lpPoolWithTagOffset0x230 + 24) = v12;
    *(_QWORD *)lpPoolWithTagOffset0x230 = 0i64;
    *(_QWORD *)(lpPoolWithTag + 168) = lpPoolWithTagOffset0x230;
    LODWORD(lpPoolWithTagOffset0x230) = lpPoolWithTagOffset0x230 + 48;
    v11 = *(_QWORD **)(lpPoolWithTag + 64);
    --dwNumCallbackNodes;
  }
  v13 = v11[49];
  if ( (v13 || v11[48]) && dwNumCallbackNodes )
    FltpInitializeCallbackNode(
      (unsigned int)lpPoolWithTagOffset0x230,
      0,
      0i64,
      (unsigned int)v11[48],
      v13,
      v11[50],
      lpPoolWithTag,
      0);
  ExReleaseCacheAwarePushLockSharedEx(v9, 0i64);
  KeLeaveGuardedRegion();
  KeEnterCriticalRegion();
  ExAcquireResourceSharedLite((PERESOURCE)(lpFltFilter + 160), 1u);
  v14 = qword_1C002A7A0;
  KeEnterGuardedRegion();
  ExAcquireCacheAwarePushLockExclusive(v14);
  if ( (*(_BYTE *)lpPoolWithTag & 1) == 0 )
  {
    v15 = 0;
    v16 = (_DWORD *)(lpFltFilter + 1088);
    v17 = (ERESOURCE *)(lpPoolWithTag + 160);
    do
    {
      if ( v17->SystemResourcesList.Flink )
      {
        FltpInsertCallback(lpPoolWithTag, lpFltFilter, v15);
        *v16 &= LODWORD(v17->SystemResourcesList.Flink[2].Blink);
      }
      ++v15;
      v17 = (ERESOURCE *)((char *)v17 + 8);
      ++v16;
    }
    while ( v15 < 0x32 );
  }
  ExReleaseCacheAwarePushLockExclusive(qword_1C002A7A0);
  KeLeaveGuardedRegion();
  ExReleaseResourceLite((PERESOURCE)(lpFltFilter + 160));
  KeLeaveCriticalRegion();
  return 0i64;
}
```
FLTMGR!FltpInitializeCallbackNode