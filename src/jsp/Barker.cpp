/*
 * JSP: https://github.com/arielm/jsp
 * COPYRIGHT (C) 2014-2015, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE SIMPLIFIED BSD LICENSE:
 * https://github.com/arielm/jsp/blob/master/LICENSE
 */

#include "jsp/Barker.h"

#if defined(JSP_USE_PRIVATE_APIS)
#include "vm/StringBuffer.h"
#endif

#include "chronotext/utils/Utils.h"

using namespace std;
using namespace chr;

namespace jsp
{
#pragma mark ---------------------------------------- barker NAMESPACE ----------------------------------------
    
    namespace barker
    {
        bool initialized = false;
        
        ptrdiff_t createCount = 0; // CLASS-INSTANCE IS NOT COUNTED
        map<ptrdiff_t, string> names;
        map<ptrdiff_t, JSObject*> instances;
        
        void setup(JSObject *instance, ptrdiff_t barkerId, const string &name = "");
        
        /*
         * RETURNS AN EMPTY-STRING IF THERE IS NO SUCH A LIVING BARKER
         * OTHERWISE: MAY RETURN THE NAME OF A NON-HEALTHY BARKER
         */
        string getName(ptrdiff_t barkerId);

        /*
         * RETURNS -1 IF THERE IS NO SUCH A LIVING BARKER
         * OTHERWISE: MAY RETURN THE ID OF A NON-HEALTHY BARKER
         */
        ptrdiff_t getId(const string &name);
        
        /*
         * IF THE RETURNED bool IS FALSE: SUCH A BARKER NEVER EXISTED
         * OTHERWISE: THE RETURNED INSTANCE MAY NOT NECESSARILY BE HEALTHY
         */
        pair<bool, JSObject*> getInstance(const string &name);
    }
    
    void barker::setup(JSObject *instance, ptrdiff_t barkerId, const string &name)
    {
        assert(barker::initialized || (barkerId == 0));
        
        for (auto &element : instances)
        {
            if (instance == element.second)
            {
                assert(false); // THIS SHOULD NEVER HAPPEN, UNLESS BARKER-FINALIZATION IS BROKEN
            }
        }
        
        // ---
        
        string finalName;
        
        if (name.empty())
        {
            finalName = "ANONYMOUS-" + ci::toString(barkerId);
        }
        else if (getId(name) >= 0)
        {
            finalName = name + " [" + ci::toString(barkerId) + "]"; // NAMES MUST BE UNIQUE (NECESSARY FOR BARKER-FORENSICS)
        }
        else
        {
            finalName = name;
        }
        
        names[barkerId] = finalName;
        
        // ---
        
        RootedObject rootedInstance(cx, instance);
        
        instances[barkerId] = instance;
        JS_SetPrivate(instance, reinterpret_cast<void*>(barkerId));
        
        RootedValue rootedId(cx, Int32Value(barkerId));
        JS_DefineProperty(cx, rootedInstance, "id", rootedId, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
        
        RootedValue rootedName(cx, toValue(finalName));
        JS_DefineProperty(cx, rootedInstance, "name", rootedName, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
        
        // ---
        
        LOGD << "Barker CONSTRUCTED: " << JSP::writeDetailed(instance) << " | " << finalName << endl; // LOG: VERBOSE
    }
    
    string barker::getName(ptrdiff_t barkerId)
    {
        auto element = names.find(barkerId);
        
        if (element != names.end())
        {
            return element->second;
        }
        
        return ""; // I.E. SUCH A BARKER NEVER EXISTED
    }
    
    ptrdiff_t barker::getId(const string &name)
    {
        for (auto &element : names)
        {
            if (element.second == name)
            {
                return element.first;
            }
        }
        
        return -1; // I.E. SUCH A BARKER NEVER EXISTED
    }
    
    pair<bool, JSObject*> barker::getInstance(const string &name)
    {
        auto barkerId = getId(name);
        
        if (barkerId >= 0)
        {
            auto instance = instances.at(barkerId); // NULL AFTER FINALIZATION
            return make_pair(true, instance);
        }
        
        return make_pair(false, nullptr); // I.E. SUCH A BARKER NEVER EXISTED
    }
    
#pragma mark ---------------------------------------- Barker NAMESPACE ----------------------------------------
    
    /*
     * IN ORDER TO PROPERLY "WITNESS" GC-MOVING:
     * BARKERS MUST BE CREATED IN THE NURSERY (JUST LIKE ANY OTHER "PLAIN" JS-OBJECT)
     *
     * ASSIGNING A FINALIZER WOULD FORCE BARKERS TO ALWAYS BE TENURED:
     * https://github.com/mozilla/gecko-dev/blob/esr31/js/public/RootingAPI.h#L318-319
     */
    
    const JSClass Barker::clazz =
    {
        "Barker",
        JSCLASS_HAS_PRIVATE,
        JS_PropertyStub,
        JS_DeletePropertyStub,
        JS_PropertyStub,
        JS_StrictPropertyStub,
        JS_EnumerateStub,
        JS_ResolveStub,
        JS_ConvertStub,
        nullptr,/*finalize*/
        nullptr,
        nullptr,
        nullptr,
        trace
    };
    
    const JSFunctionSpec Barker::functions[] =
    {
        JS_FS("toSource", function_toSource, 0, 0),
        JS_FS("bark", function_bark, 0, 0),
        JS_FS_END
    };
    
    const JSFunctionSpec Barker::static_functions[] =
    {
        JS_FS("getInstance", static_function_getInstance, 1, 0),
        JS_FS("isFinalized", static_function_isFinalized, 1, 0),
        JS_FS("isHealthy", static_function_isHealthy, 1, 0),
        JS_FS("bark", static_function_bark, 1, 0),
        JS_FS_END
    };
    
    // ---
    
    void Barker::gcCallback(JSRuntime *rt, JSGCStatus status)
    {
        switch (status)
        {
            case JSGC_BEGIN:
                break;
                
            case JSGC_END:
            {
                auto freeOp = rt->defaultFreeOp();
                
                for (auto &element : barker::instances)
                {
                    if (element.second)
                    {
                        if (!JSP::isHealthy(element.second))
                        {
                            finalize(freeOp, element.second);
                        }
                    }
                }
                
                break;
            }
        }
    }
    
    void Barker::finalize(JSFreeOp *fop, JSObject *obj)
    {
        auto barkerId = -1;
        
        if (obj)
        {
            for (auto &element : barker::instances)
            {
                if (obj == element.second)
                {
                    barkerId = element.first;
                    break;
                }
            }
        }
        
        if (barkerId >= 0)
        {
            /*
             * NECESSARY FOR BARKER-FORENSICS:
             *
             * - NULLING (I.E. NOT ERASING) ENTRY IN barker::instances
             * - PRESERVING ENTRY IN barker::names
             */
            barker::instances[barkerId] = nullptr;
            
            LOGD << "Barker FINALIZED: " << JSP::writeDetailed(obj) << " | " << barker::getName(barkerId) << endl; // LOG: VERBOSE
        }
    }
    
    void Barker::trace(JSTracer *trc, JSObject *obj)
    {
        auto barkerId = getId(obj);
        
        if (barkerId >= 0)
        {
            /*
             * NECESSARY IN CASE OF MOVED-POINTER
             */
            barker::instances[barkerId] = obj;
            
            if (barkerId > 0) // EXCLUDING THE CLASS-INSTANCE
            {
                LOGD << "Barker TRACED: " << JSP::writeDetailed(obj) << " | " << barker::getName(barkerId) << endl; // LOG: VERBOSE
            }
        }
    }
    
    // ---
    
    int32_t Barker::nextId()
    {
        return int32_t(barker::createCount) + 1;
    }
    
    ptrdiff_t Barker::getId(JSObject *instance)
    {
        if (JSP::isHealthy(instance))
        {
            if (JS_GetClass(instance) == &Barker::clazz)
            {
                auto barkerId = reinterpret_cast<ptrdiff_t>(JS_GetPrivate(instance));
                
                if (barker::instances.count(barkerId))
                {
                    return barkerId;
                }
            }
        }
        
        return -1; // I.E. THERE IS NO SUCH A LIVING BARKER
    }
    
    string Barker::getName(JSObject *instance)
    {
        return barker::getName(getId(instance));
    }
    
    JSObject* Barker::getInstance(const string &name)
    {
        bool found;
        JSObject *instance;
        tie(found, instance) = barker::getInstance(name);
        
        if (found)
        {
            if (JSP::isHealthy(instance))
            {
                return instance;
            }
        }
        
        return nullptr; // I.E. THERE IS NO SUCH A LIVING BARKER
    }
    
    bool Barker::isFinalized(const string &name)
    {
        bool found;
        JSObject *instance;
        tie(found, instance) = barker::getInstance(name);
        
        if (found)
        {
            return !JSP::isHealthy(instance);
        }
        
        return false;
    }
    
    bool Barker::isHealthy(const string &name)
    {
        bool found;
        JSObject *instance;
        tie(found, instance) = barker::getInstance(name);
        
        if (found)
        {
            return JSP::isHealthy(instance);
        }
        
        return false;
    }
    
    // ---
    
    bool Barker::bark(JSObject *object)
    {
        return maybeBark(object);
    }
    
    bool Barker::bark(const Value &value)
    {
        if (value.isObject())
        {
            return maybeBark(value.toObjectOrNull());
        }
        
        return false;
    }
    
    bool Barker::bark(const string &name)
    {
        bool found;
        JSObject *instance;
        tie(found, instance) = barker::getInstance(name);
        
        if (found)
        {
            return maybeBark(instance);
        }
        
        return false;
    }
    
    bool Barker::maybeBark(JSObject *instance)
    {
        auto barkerId = getId(instance);
        
        if (barkerId >= 0)
        {
            LOGD << "Barker BARKED: " << JSP::writeDetailed(instance) << " | " << barker::getName(barkerId) << endl; // LOG: VERBOSE
            return true;
        }
        
        LOGD << "ONLY HEALTHY BARKERS CAN BARK" << endl; // LOG: VERBOSE
        return false;
    }
    
    // ---
    
    bool Barker::init()
    {
        if (!barker::initialized)
        {
            auto classInstance = JS_InitClass(cx, globalHandle(), NullPtr(), &clazz, construct, 0, nullptr, functions, nullptr, static_functions);
            barker::setup(classInstance, 0, "CLASS-INSTANCE");
            
            static bool unique;
            addGCCallback(&unique, BIND_STATIC2(Barker::gcCallback));
            
            // ---
            
            barker::initialized = true;
        }
        
        return barker::initialized;
    }
    
    /*
     * TODO: COMPLETE UN-INITIALIZATION
     */
    void Barker::uninit()
    {
        barker::initialized = false;
    }
    
    // ---
    
    const Barker& Barker::create(const string &name)
    {
        static Barker delegate;

        delegate.object = JS_NewObject(cx, &clazz, NullPtr(), NullPtr());
        barker::setup(delegate.object, ++barker::createCount, name);
        
        return delegate;
    }
    
    bool Barker::construct(JSContext *cx, unsigned argc, Value *vp)
    {
        auto args = CallArgsFromVp(argc, vp);
        string name = (args.hasDefined(0) && args[0].isString()) ? toString(args[0]) : "";
        
        args.rval().set(create(name));
        return true;
    }
    
    // ---
    
    bool Barker::function_toSource(JSContext *cx, unsigned argc, Value *vp)
    {
        auto args = CallArgsFromVp(argc, vp);
        auto instance = args.thisv().toObjectOrNull();
        
        char quoteChar = '"';
        string quotedName = quoteChar + getName(instance) + quoteChar;
        
        js::StringBuffer sb(cx);
        
        if (sb.append("(new Barker(") && sb.appendInflated(quotedName.data(), quotedName.size()) && sb.append("))"))
        {
            JSString *str = sb.finishString();
            
            if (str)
            {
                args.rval().setString(str);
                return true;
            }
        }
        
        return false;
    }
    
    bool Barker::function_bark(JSContext *cx, unsigned argc, Value *vp)
    {
        auto args = CallArgsFromVp(argc, vp);
        auto instance = args.thisv().toObjectOrNull();
        
        args.rval().setBoolean(maybeBark(instance));
        return true;
    }
    
    // ---
    
    bool Barker::static_function_getInstance(JSContext *cx, unsigned argc, Value *vp)
    {
        auto args = CallArgsFromVp(argc, vp);
        
        if (args.hasDefined(0) && args[0].isString())
        {
            bool found;
            JSObject *instance;
            tie(found, instance) = barker::getInstance(toString(args[0]));
            
            if (found)
            {
                args.rval().setObjectOrNull(instance);
                return true;
            }
        }
        
        return false;
    }
    
    bool Barker::static_function_isFinalized(JSContext *cx, unsigned argc, Value *vp)
    {
        auto args = CallArgsFromVp(argc, vp);
        
        if (args.hasDefined(0) && args[0].isString())
        {
            args.rval().setBoolean(isFinalized(toString(args[0])));
            return true;
        }
        
        return false;
    }
    
    bool Barker::static_function_isHealthy(JSContext *cx, unsigned argc, Value *vp)
    {
        auto args = CallArgsFromVp(argc, vp);
        
        if (args.hasDefined(0) && args[0].isString())
        {
            args.rval().setBoolean(isHealthy(toString(args[0])));
            return true;
        }
        
        return false;
    }
    
    bool Barker::static_function_bark(JSContext *cx, unsigned argc, Value *vp)
    {
        auto args = CallArgsFromVp(argc, vp);
        
        if (args.hasDefined(0) && args[0].isString())
        {
            args.rval().setBoolean(bark(toString(args[0])));
            return true;
        }
        
        return false;
    }
}
