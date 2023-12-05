#ifndef bridge_h
#define bridge_h

#include "DumbHostObject.h"
#include "SmartHostObject.h"
#include "types.h"
#include "utils.h"
#include <sqlite3.h>
#include <vector>

namespace opsqlite {

namespace jsi = facebook::jsi;

BridgeResult sqliteOpenDb(std::string const dbName, std::string const dbPath);

BridgeResult sqliteCloseDb(std::string const dbName);

BridgeResult sqliteRemoveDb(std::string const dbName,
                            std::string const docPath);

BridgeResult sqliteAttachDb(std::string const mainDBName,
                            std::string const docPath,
                            std::string const databaseToAttach,
                            std::string const alias);

BridgeResult sqliteDetachDb(std::string const mainDBName,
                            std::string const alias);

BridgeResult
sqliteExecute(std::string const dbName, std::string const &query,
              const std::vector<JSVariant> *params,
              std::vector<DumbHostObject> *results,
              std::shared_ptr<std::vector<SmartHostObject>> metadatas);

BridgeResult sqliteExecuteLiteral(std::string const dbName,
                                  std::string const &query);

void sqliteCloseAll();

BridgeResult registerUpdateHook(
    std::string const dbName,
    std::function<void(std::string dbName, std::string tableName,
                       std::string operation, int rowId)> const callback);
BridgeResult unregisterUpdateHook(std::string const dbName);
BridgeResult
registerCommitHook(std::string const dbName,
                   std::function<void(std::string dbName)> const callback);
BridgeResult unregisterCommitHook(std::string const dbName);
BridgeResult
registerRollbackHook(std::string const dbName,
                     std::function<void(std::string dbName)> const callback);
BridgeResult unregisterRollbackHook(std::string const dbName);

sqlite3_stmt *sqlite_prepare_statement(std::string const dbName,
                                       std::string const &query);
} // namespace opsqlite

#endif /* bridge_h */
