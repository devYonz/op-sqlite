#include "bindings.h"
#include "bridge.h"
#include "logs.h"
#include "utils.h"
#include "ThreadPool.h"
#include "sqlbatchexecutor.h"
#include <vector>
#include <string>
#include "macros.h"
#include <iostream>

namespace osp {

namespace jsi = facebook::jsi;

std::string basePath;
std::shared_ptr<react::CallInvoker> invoker;
std::shared_ptr<ThreadPool> pool;

void clearState() {
    sqliteCloseAll();
}

void install(jsi::Runtime &rt, std::shared_ptr<react::CallInvoker> jsCallInvoker, const char *docPath)
{
    basePath = std::string(docPath);
    pool = std::make_shared<ThreadPool>();
    invoker = jsCallInvoker;
    
    auto open = HOSTFN("open", 2) {
        if (count == 0)
        {
            throw jsi::JSError(rt, "[op-sqlite][open] database name is required");
        }
        
        if (!args[0].isString())
        {
            throw jsi::JSError(rt, "[op-sqlite][open] database name must be a string");
        }
        
        std::string dbName = args[0].asString(rt).utf8(rt);
        std::string tempDocPath = std::string(basePath);
        
        if (count > 1 && !args[1].isUndefined() && !args[1].isNull())
        {
            if (!args[1].isString())
            {
                throw jsi::JSError(rt, "[op-sqlite][open] database location must be a string");
            }
            
            tempDocPath = tempDocPath + "/" + args[1].asString(rt).utf8(rt);
        }
        
        BridgeResult result = sqliteOpenDb(dbName, tempDocPath);
        
        if (result.type == SQLiteError)
        {
            throw jsi::JSError(rt, result.message.c_str());
        }
        
        return {};
    });
    
    auto attach = HOSTFN("attach", 4) {
        if(count < 3) {
            throw jsi::JSError(rt, "[op-sqlite][attach] Incorrect number of arguments");
        }
        if (!args[0].isString() || !args[1].isString() || !args[2].isString())
        {
            throw jsi::JSError(rt, "dbName, databaseToAttach and alias must be a strings");
            return {};
        }
        
        std::string tempDocPath = std::string(basePath);
        if (count > 3 && !args[3].isUndefined() && !args[3].isNull())
        {
            if (!args[3].isString())
            {
                throw jsi::JSError(rt, "[op-sqlite][attach] database location must be a string");
            }
            
            tempDocPath = tempDocPath + "/" + args[3].asString(rt).utf8(rt);
        }
        
        std::string dbName = args[0].asString(rt).utf8(rt);
        std::string databaseToAttach = args[1].asString(rt).utf8(rt);
        std::string alias = args[2].asString(rt).utf8(rt);
        BridgeResult result = sqliteAttachDb(dbName, tempDocPath, databaseToAttach, alias);
        
        if (result.type == SQLiteError)
        {
            throw jsi::JSError(rt, result.message.c_str());
        }
        
        return {};
    });
    
    auto detach = HOSTFN("detach", 2) {
        if(count < 2) {
            throw jsi::JSError(rt, "[op-sqlite][detach] Incorrect number of arguments");
        }
        if (!args[0].isString() || !args[1].isString())
        {
            throw jsi::JSError(rt, "dbName, databaseToAttach and alias must be a strings");
            return {};
        }
        
        std::string dbName = args[0].asString(rt).utf8(rt);
        std::string alias = args[1].asString(rt).utf8(rt);
        BridgeResult result = sqliteDetachDb(dbName, alias);
        
        if (result.type == SQLiteError)
        {
            throw jsi::JSError(rt, result.message.c_str());
        }
        
        return {};
    });
    
    auto close = HOSTFN("close", 1)
    {
        if (count == 0)
        {
            throw jsi::JSError(rt, "[op-sqlite][close] database name is required");
        }
        
        if (!args[0].isString())
        {
            throw jsi::JSError(rt, "[op-sqlite][close] database name must be a string");
        }
        
        std::string dbName = args[0].asString(rt).utf8(rt);
        
        BridgeResult result = sqliteCloseDb(dbName);
        
        if (result.type == SQLiteError)
        {
            throw jsi::JSError(rt, result.message.c_str());
        }
        
        return {};
    });
    
    auto remove = HOSTFN("delete", 2)
    {
        if (count == 0)
        {
            throw jsi::JSError(rt, "[op-sqlite][open] database name is required");
        }
        
        if (!args[0].isString())
        {
            throw jsi::JSError(rt, "[op-sqlite][open] database name must be a string");
        }
        
        std::string dbName = args[0].asString(rt).utf8(rt);
        
        std::string tempDocPath = std::string(basePath);
        
        if (count > 1 && !args[1].isUndefined() && !args[1].isNull())
        {
            if (!args[1].isString())
            {
                throw jsi::JSError(rt, "[op-sqlite][open] database location must be a string");
            }
            
            tempDocPath = tempDocPath + "/" + args[1].asString(rt).utf8(rt);
        }
        
        
        BridgeResult result = sqliteRemoveDb(dbName, tempDocPath);
        
        if (result.type == SQLiteError)
        {
            throw jsi::JSError(rt, result.message.c_str());
        }
        
        return {};
    });
    
    auto execute = HOSTFN("execute", 3)
    {
        const std::string dbName = args[0].asString(rt).utf8(rt);
        const std::string query = args[1].asString(rt).utf8(rt);
        std::vector<std::any> params;
        if(count == 3) {
            const jsi::Value &originalParams = args[2];
            params = jsiQueryArgumentsToSequelParam(rt, originalParams);
        }
        
        std::vector<std::shared_ptr<DynamicHostObject>> results;
        std::vector<QuickColumnMetadata> metadata;
        
        try {
            auto status = sqliteExecute(dbName, query, &params, &results, &metadata);
            
            if(status.type == SQLiteError) {
                throw jsi::JSError(rt, status.message);
            }
            
            auto jsiResult = createResult(rt, status, &results, &metadata);
            return jsiResult;
        } catch(std::exception &e) {
            throw jsi::JSError(rt, e.what());
        }
    });
    
    auto executeAsync = HOSTFN("executeAsync", 3)
    {
        if (count < 3)
        {
            throw jsi::JSError(rt, "[op-sqlite][executeAsync] Incorrect arguments for executeAsync");
        }
        
        const std::string dbName = args[0].asString(rt).utf8(rt);
        const std::string query = args[1].asString(rt).utf8(rt);
        const jsi::Value &originalParams = args[2];
        
        std::vector<std::any> params = jsiQueryArgumentsToSequelParam(rt, originalParams);
        
        auto promiseCtr = rt.global().getPropertyAsFunction(rt, "Promise");
        auto promise = promiseCtr.callAsConstructor(rt, HOSTFN("executor", 2) {
            auto resolve = std::make_shared<jsi::Value>(rt, args[0]);
            auto reject = std::make_shared<jsi::Value>(rt, args[1]);
            
            auto task =
            [&rt, dbName, query, params = std::make_shared<std::vector<std::any>>(params), resolve, reject]()
            {
                try
                {
                    std::vector<std::shared_ptr<DynamicHostObject>> results;
                    std::vector<QuickColumnMetadata> metadata;
                    auto status = sqliteExecute(dbName, query, params.get(), &results, &metadata);
                    
                    invoker->invokeAsync([&rt, results = std::make_shared<std::vector<std::shared_ptr<DynamicHostObject>>>(results), metadata = std::make_shared<std::vector<QuickColumnMetadata>>(metadata), status_copy = std::move(status), resolve, reject] {
                        if(status_copy.type == SQLiteOk) {
                            auto jsiResult = createResult(rt, status_copy, results.get(), metadata.get());
                            resolve->asObject(rt).asFunction(rt).call(rt, std::move(jsiResult));
                        } else {
                            auto errorCtr = rt.global().getPropertyAsFunction(rt, "Error");
                            auto error = errorCtr.callAsConstructor(rt, jsi::String::createFromUtf8(rt, status_copy.message));
                            reject->asObject(rt).asFunction(rt).call(rt, error);
                        }
                    });
                    
                }
                catch (std::exception &exc)
                {
                    invoker->invokeAsync([&rt, &exc] {
                        jsi::JSError(rt, exc.what());
                    });
                }
            };
            
            pool->queueWork(task);
            
            return {};
        }));
        
        return promise;
    });
    
    // Execute a batch of SQL queries in a transaction
    // Parameters can be: [[sql: string, arguments: any[] | arguments: any[][] ]]
    auto executeBatch = HOSTFN("executeBatch", 2)
    {
        if (sizeof(args) < 2)
        {
            throw jsi::JSError(rt, "[op-sqlite][executeBatch] - Incorrect parameter count");
        }
        
        const jsi::Value &params = args[1];
        if (params.isNull() || params.isUndefined())
        {
            throw jsi::JSError(rt, "[op-sqlite][executeBatch] - An array of SQL commands or parameters is needed");
        }
        const std::string dbName = args[0].asString(rt).utf8(rt);
        const jsi::Array &batchParams = params.asObject(rt).asArray(rt);
        std::vector<QuickQueryArguments> commands;
        jsiBatchParametersToQuickArguments(rt, batchParams, &commands);
        
        auto batchResult = sqliteExecuteBatch(dbName, &commands);
        if (batchResult.type == SQLiteOk)
        {
            auto res = jsi::Object(rt);
            res.setProperty(rt, "rowsAffected", jsi::Value(batchResult.affectedRows));
            return std::move(res);
        }
        else
        {
            throw jsi::JSError(rt, batchResult.message);
        }
    });
    
    auto executeBatchAsync = HOSTFN("executeBatchAsync", 2)
    {
        if (sizeof(args) < 2)
        {
            throw jsi::JSError(rt, "[op-sqlite][executeAsyncBatch] Incorrect parameter count");
            return {};
        }
        
        const jsi::Value &params = args[1];
        
        if (params.isNull() || params.isUndefined())
        {
            throw jsi::JSError(rt, "[op-sqlite][executeAsyncBatch] - An array of SQL commands or parameters is needed");
            return {};
        }
        
        const std::string dbName = args[0].asString(rt).utf8(rt);
        const jsi::Array &batchParams = params.asObject(rt).asArray(rt);
        
        std::vector<QuickQueryArguments> commands;
        jsiBatchParametersToQuickArguments(rt, batchParams, &commands);
        
        auto promiseCtr = rt.global().getPropertyAsFunction(rt, "Promise");
        auto promise = promiseCtr.callAsConstructor(rt, HOSTFN("executor", 2) {
            auto resolve = std::make_shared<jsi::Value>(rt, args[0]);
            auto reject = std::make_shared<jsi::Value>(rt, args[1]);
            
            auto task =
            [&rt, dbName, commands = std::make_shared<std::vector<QuickQueryArguments>>(commands), resolve, reject]()
            {
                try
                {
                    // Inside the new worker thread, we can now call sqlite operations
                    auto batchResult = sqliteExecuteBatch(dbName, commands.get());
                    invoker->invokeAsync([&rt, batchResult = std::move(batchResult), resolve, reject]
                                         {
                        if(batchResult.type == SQLiteOk)
                        {
                            auto res = jsi::Object(rt);
                            res.setProperty(rt, "rowsAffected", jsi::Value(batchResult.affectedRows));
                            resolve->asObject(rt).asFunction(rt).call(rt, std::move(res));
                        } else
                        {
                            throw jsi::JSError(rt, batchResult.message);
                        } });
                }
                catch (std::exception &exc)
                {
                    invoker->invokeAsync([&rt, reject, &exc]
                                         {
                        throw jsi::JSError(rt, exc.what());
                    });
                }
            };
            pool->queueWork(task);
            
            return {};
        }));
        
        return promise;
    });
    
    auto loadFile = HOSTFN("loadFile", 2)
    {
        if (sizeof(args) < 2)
        {
            throw jsi::JSError(rt, "[op-sqlite][loadFileAsync] Incorrect parameter count");
            return {};
        }
        
        const std::string dbName = args[0].asString(rt).utf8(rt);
        const std::string sqlFileName = args[1].asString(rt).utf8(rt);
        
        auto promiseCtr = rt.global().getPropertyAsFunction(rt, "Promise");
        auto promise = promiseCtr.callAsConstructor(rt, HOSTFN("executor", 2) {
            auto resolve = std::make_shared<jsi::Value>(rt, args[0]);
            auto reject = std::make_shared<jsi::Value>(rt, args[1]);
            
            auto task =
            [&rt, dbName, sqlFileName, resolve, reject]()
            {
                try
                {
                    const auto importResult = importSQLFile(dbName, sqlFileName);
                    
                    invoker->invokeAsync([&rt, result = std::move(importResult), resolve, reject]
                                         {
                        if(result.type == SQLiteOk)
                        {
                            auto res = jsi::Object(rt);
                            res.setProperty(rt, "rowsAffected", jsi::Value(result.affectedRows));
                            res.setProperty(rt, "commands", jsi::Value(result.commands));
                            resolve->asObject(rt).asFunction(rt).call(rt, std::move(res));
                        } else {
                            throw jsi::JSError(rt, result.message);
                        } });
                }
                catch (std::exception &exc)
                {
                    invoker->invokeAsync([&rt, err = exc.what(), reject]
                                         {
                        throw jsi::JSError(rt, err);
                    });
                }
            };
            pool->queueWork(task);
            return {};
        }));
        
        return promise;
    });
    
    
    
    jsi::Object module = jsi::Object(rt);
    
    module.setProperty(rt, "open", std::move(open));
    module.setProperty(rt, "close", std::move(close));
    module.setProperty(rt, "attach", std::move(attach));
    module.setProperty(rt, "detach", std::move(detach));
    module.setProperty(rt, "delete", std::move(remove));
    module.setProperty(rt, "execute", std::move(execute));
    module.setProperty(rt, "executeAsync", std::move(executeAsync));
    module.setProperty(rt, "executeBatch", std::move(executeBatch));
    module.setProperty(rt, "executeBatchAsync", std::move(executeBatchAsync));
    module.setProperty(rt, "loadFile", std::move(loadFile));
    
    rt.global().setProperty(rt, "__OPSQLiteProxy", std::move(module));
}

}
