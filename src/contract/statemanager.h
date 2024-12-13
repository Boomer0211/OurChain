#ifndef CONTRACT_MANAGER_H
#define CONTRACT_MANAGER_H

#include "chain.h"
#include "contract/cache.h"
#include "contract/updatepolicy.h"
#include "primitives/transaction.h"
#include "util.h"
#include "validation.h"

class ContractStateManager
{
public:
    ContractStateManager(ContractStateCache* cache);
    // 當區塊鏈狀態改變時觸發, 用於更新合約狀態快照
    bool SyncState(CChain& chainActive, const Consensus::Params consensusParams);

private:
    ContractStateCache* cache;

    bool isSaveCheckPointNow(int height);
    bool isClearCheckPointNow(int height);
};

#endif // CONTRACT_MANAGER_H