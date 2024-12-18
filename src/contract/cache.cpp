#include "contract/cache.h"

SnapShot::SnapShot(std::string name)
{
    dbWrapper = new ContractDBWrapper(name);
}
SnapShot::~SnapShot()
{
    delete dbWrapper;
}

void SnapShot::setContractState(uint256 address, json state)
{
    dbWrapper->setState(address.ToString(), state.dump());
    assert(dbWrapper->isOk());
}
json SnapShot::getContractState(std::string address)
{
    std::string state = dbWrapper->getState(address);
    if (dbWrapper->isOk() == false)
        return nullptr;
    return json::parse(state);
}
void SnapShot::clear()
{
    dbWrapper->clearAllStates();
}
void SnapShot::saveCheckPoint(std::string tipBlockHash)
{
    dbWrapper->saveCheckPoint(tipBlockHash);
}
ContractDBWrapper* SnapShot::getDBWrapper()
{
    return dbWrapper;
}

BlockCache::BlockCache()
{
    auto path = std::string("block_index");
    dbWrapper = new ContractDBWrapper(path);
}
BlockCache::~BlockCache()
{
    delete dbWrapper;
}

void BlockCache::clear()
{
    dbWrapper->clearAllStates();
}
void BlockCache::setBlockIndex(uint256 blockHash, int blockHeight)
{
    dbWrapper->setState(intToKey(blockHeight), blockHash.ToString());
    assert(dbWrapper->isOk());
}
uint256 BlockCache::getBlockHash(int blockHeight)
{
    std::string blockHash = dbWrapper->getState(intToKey(blockHeight));
    if (dbWrapper->isOk() == false)
        return uint256();
    return uint256S(blockHash);
}
BlockCache::blockIndex BlockCache::getHeighestBlock()
{
    rocksdb::Iterator* it = dbWrapper->getIterator();
    // 定位到数据库的最后一个条目
    it->SeekToLast();
    if (it->Valid() == false) {
        delete it;
        return blockIndex{uint256(), -1};
    }
    blockIndex result = blockIndex(uint256S(it->value().ToString()), keyToInt(it->key().ToString()));
    delete it;
    return result;
}
void BlockCache::removeBlockIndex(int blockHeight)
{
    dbWrapper->deleteState(intToKey(blockHeight));
    assert(dbWrapper->isOk());
}

ContractStateCache::ContractStateCache()
{
    blockcache = new BlockCache();
    snapshot = new SnapShot(std::string("current_cache"));
}
ContractStateCache::~ContractStateCache()
{
    delete blockcache;
    delete snapshot;
}
SnapShot* ContractStateCache::getSnapShot()
{
    return snapshot;
}
void ContractStateCache::clearSnapShot()
{
    snapshot->clear();
}
bool ContractStateCache::getFirstBlockCache(BlockCache::blockIndex& blockIndex)
{
    blockIndex = blockcache->getHeighestBlock();
    if (blockIndex.blockHeight == -1)
        return false;
    return true;
}
BlockCache* ContractStateCache::getBlockCache()
{
    return blockcache;
}
void ContractStateCache::pushBlock(BlockCache::blockIndex blockIndex)
{
    blockcache->setBlockIndex(blockIndex.blockHash, blockIndex.blockHeight);
}
void ContractStateCache::popBlock()
{
    BlockCache::blockIndex blockIndex = blockcache->getHeighestBlock();
    blockcache->removeBlockIndex(blockIndex.blockHeight);
}
void ContractStateCache::saveCheckPoint()
{
    auto blockIndex = blockcache->getHeighestBlock();
    snapshot->saveCheckPoint(blockIndex.blockHash.ToString());
}
bool ContractStateCache::restoreCheckPoint(std::string tipBlockHash, std::vector<CheckPointInfo> checkPointList)
{
    // get target beckup id by tipBlockHash
    int targetBackupId = -1;
    for (auto it = checkPointList.begin(); it != checkPointList.end(); it++) {
        if (it->tipBlockHash == tipBlockHash) {
            targetBackupId = it->id;
            break;
        }
    }
    assert(targetBackupId > 0);
    return this->snapshot->getDBWrapper()->restoreCheckPoint(targetBackupId);
}
void ContractStateCache::clearCheckPoint(int maxBlockCheckPointCount)
{
    this->snapshot->getDBWrapper()->removeOldCheckPoint(maxBlockCheckPointCount);
}

fs::path ContractStateCache::getContractPath(std::string name)
{
    return snapshot->getDBWrapper()->getContractDBPath(name);
}

std::vector<CheckPointInfo> ContractStateCache::getCheckPointList()
{
    return snapshot->getDBWrapper()->findCheckPointList();
}