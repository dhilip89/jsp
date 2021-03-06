/*
 * JSP: https://github.com/arielm/jsp
 * COPYRIGHT (C) 2014-2015, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE SIMPLIFIED BSD LICENSE:
 * https://github.com/arielm/jsp/blob/master/LICENSE
 */

/*
 * BARKER BASED ON: js/src/jsapi-tests/testPersistentRooted.cpp
 *
 * FOLLOW-UP: IT WAS A PRETTY MISLEADING CODE-SAMPLE!
 *
 * - NOT ADAPTED TO "GENERATIONAL GC"
 * - FOCUSED ON TRYING TO AVOID "STRAY INSTANCES" ON THE C-STACK (A PURE "INCREMENTAL GC" TOPIC)
 * - USING A CUSTOM CLASS FINALIZER:
 *   - THE LAST THING TO DO WHEN STUDYING "GENERATIONAL GC":
 *     - SUCH OBJECTS ARE CREATED DIRECTLY ON THE THE "TENURED HEAP"!
 *     - I.E. IT WAS THEREFORE IMPOSSIBLE TO WITNESS "MOVING FROM THE NURSERY"
 *
 *
 * TODO:
 *
 * 1) IT WOULD BE BETTER TO HAVE: barkers['some barker name']
 *    INSTEAD OF AS CURRENTLY: Barker.getInstance('some barker name')
 *    HINT: jsapi-tests/testClassGetter.cpp
 *
 * 2) TEST Barker::init() / Barker::uninit() FURTHER
 *
 * 3) ALLOW TO ENABLE/DISABLE LOGGING
 *
 * 4) CONSIDER USING A CUSTOM NATIVE OBJECT EXTENDING JSObject:
 *    - https://github.com/mozilla/gecko-dev/blob/esr31/js/src/builtin/TestingFunctions.cpp#L1262-1421
 *    - ALTERNATIVELY:
 *      - CHECK THE "Helper Macros for creating JSClasses that function as proxies" (EG. "PROXY_CLASS_WITH_EXT") IN jsfriendapi.h
 *      - EXAMPLE IN jsapi-tests/testBug604087.cpp
 */

#pragma once

#include "jsp/Context.h"

namespace jsp
{
    class Barker
    {
    public:
        operator JSObject* () const
        {
            return object;
        }
        
        operator Value () const
        {
            return ObjectOrNullValue(object);
        }

        // ---
        
        static int32_t nextId();
        
        static int32_t getId(JSObject *instance); // RETURNS -1 IF THERE IS NO SUCH A LIVING BARKER
        static std::string getName(JSObject *instance); // RETURNS AN EMPTY-STRING IF THERE IS NO SUCH A LIVING BARKER
        static JSObject* getInstance(const std::string &name); // RETURNS NULL IF THERE IS NO SUCH A LIVING BARKER
        
        static bool isFinalized(const std::string &name); // I.E. ONCE A BARKER, NOW DEAD
        static bool isHealthy(const std::string &name); // I.E. IT'S A BARKER, AND IT'S ALIVE!
        
        /*
         * ONLY HEALTHY BARKERS CAN BARK
         */
        static bool bark(JSObject *object);
        static bool bark(const Value &value);
        static bool bark(const std::string &name);
        
        // ---

        static bool init();
        static void uninit();
        
        /*
         * C++ FACTORY
         */
        static const Barker& create(const std::string &name = "");
        
    private:
        JSObject* object = nullptr;
        
        Barker() = default;
        Barker(const Barker &other) = delete;
        void operator=(const Barker &other) = delete;

        // ---
        
        struct Statics
        {
            JS::Heap<JSObject*> prototype;
            
            std::map<int32_t, std::string> names;
            std::map<int32_t, JSObject*> instances;
        };
        
        static Statics *statics;
        static int32_t lastInstanceId;
        
        static int32_t addInstance(JSObject *instance, const std::string &name = "");
        static std::string getName(int32_t instanceId);
        static int32_t getId(const std::string &name);
        static std::pair<bool, JSObject*> find(const std::string &name);
        
        // ---
        
        static const JSClass clazz;
        static const JSFunctionSpec functions[];
        static const JSFunctionSpec static_functions[];

        /*
         * JS CONSTRUCTOR
         */
        static bool construct(JSContext *cx, unsigned argc, Value *vp);
        
        static bool function_toSource(JSContext *cx, unsigned argc, Value *vp);
        static bool function_bark(JSContext *cx, unsigned argc, Value *vp);
        
        static bool static_function_getInstance(JSContext *cx, unsigned argc, Value *vp);
        static bool static_function_isFinalized(JSContext *cx, unsigned argc, Value *vp);
        static bool static_function_isHealthy(JSContext *cx, unsigned argc, Value *vp);
        static bool static_function_bark(JSContext *cx, unsigned argc, Value *vp);

        // ---
        
        static void gcCallback(JSRuntime *rt, JSGCStatus status);
        static void finalize(JSFreeOp *fop, JSObject *obj);
        static void trace(JSTracer *trc, JSObject *obj);
        
        static bool maybeBark(JSObject *instance);
    };
}
