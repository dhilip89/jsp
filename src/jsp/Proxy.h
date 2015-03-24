/*
 * JSP: https://github.com/arielm/jsp
 * COPYRIGHT (C) 2014-2015, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE SIMPLIFIED BSD LICENSE:
 * https://github.com/arielm/jsp/blob/master/LICENSE
 */

#pragma once

#include "jsp/Proto.h"

#include <map>

#define TARGET(FN, ...) target->FN(__VA_ARGS__)
#define HANDLE(FN, ...) handler->FN(__VA_ARGS__)
#define FORWARD(FN, ...) handler ? HANDLE(FN, __VA_ARGS__) : TARGET(FN, __VA_ARGS__)

#define BIND_STATIC_CALLBACK(CALLABLE) std::bind(CALLABLE, std::placeholders::_1)
#define BIND_INSTANCE_CALLBACK(CALLABLE, INSTANCE) std::bind(CALLABLE, INSTANCE, std::placeholders::_1)

namespace jsp
{
    struct Callback
    {
        std::string name;
        CallbackFn fn;
        
        Callback(const std::string &name, const CallbackFn &fn)
        :
        name(name),
        fn(fn)
        {}
    };
    
    class Proxy : public Proto
    {
    public:
        Proxy(Proto *target);
        Proxy(Proxy *target);
        
        Proxy();
        ~Proxy();

        bool setTarget(Proto *target);
        bool setHandler(Proto *handler);

        bool setTarget(Proxy *target);
        bool setHandler(Proxy *handler);

        // ---
        
        inline bool exec(const std::string &source, const ReadOnlyCompileOptions &options) final
        {
            return FORWARD(exec, source, options);
        }
        
        inline bool eval(const std::string &source, const ReadOnlyCompileOptions &options, MutableHandleValue result) final
        {
            return FORWARD(eval, source, options, result);
        }
        
        // ---
        
        inline Value call(HandleObject object, const char *functionName, const HandleValueArray& args = HandleValueArray::empty()) final
        {
            return FORWARD(call, object, functionName, args);
        }
        
        inline Value call(HandleObject object, HandleValue functionValue, const HandleValueArray& args = HandleValueArray::empty()) final
        {
            return FORWARD(call, object, functionValue, args);
        }
        
        inline Value call(HandleObject object, HandleFunction function, const HandleValueArray& args = HandleValueArray::empty()) final
        {
            return FORWARD(call, object, function, args);
        }
        
        // ---
        
        inline bool applyCallback(const CallbackFn &fn, CallArgs args) final
        {
            return FORWARD(applyCallback, fn, args);
        }
        
        // ---
        
        inline JSObject* newObject() final
        {
            return FORWARD(newObject);
        }
        
        inline bool hasProperty(HandleObject object, const char *name) final
        {
            return FORWARD(hasProperty, object, name);
        }
        
        inline bool hasOwnProperty(HandleObject object, const char *name) final
        {
            return FORWARD(hasOwnProperty, object, name);
        }
        
        inline bool getProperty(HandleObject object, const char *name, MutableHandleValue result) final
        {
            return FORWARD(getProperty, object, name, result);
        }
        
        inline bool setProperty(HandleObject object, const char *name, HandleValue value) final
        {
            return FORWARD(setProperty, object, name, value);
        }
        
        inline bool deleteProperty(HandleObject object, const char *name) final
        {
            return FORWARD(deleteProperty, object, name);
        }
        
        inline bool getOwnPropertyDescriptor(HandleObject object, HandleId id, MutableHandle<JSPropertyDescriptor> desc) final
        {
            return FORWARD(getOwnPropertyDescriptor, object, id, desc);
        }
        
        // ---
        
        inline JSObject* newArray(size_t length = 0) final
        {
            return FORWARD(newArray, length);
        }
        
        inline JSObject* newArray(const HandleValueArray& contents) final
        {
            return FORWARD(newArray, contents);
        }
        
        inline uint32_t getLength(HandleObject array) final
        {
            return FORWARD(getLength, array);
        }
        
        inline bool getElement(HandleObject array, uint32_t index, MutableHandleValue result) final
        {
            return FORWARD(getElement, array, index, result);
        }
        
        inline bool setElement(HandleObject array, uint32_t index, HandleValue value) final
        {
            return FORWARD(setElement, array, index, value);
        }
        
        inline bool deleteElement(HandleObject array, uint32_t index) final
        {
            return FORWARD(deleteElement, array, index);
        }

    protected:
        Proto *target = nullptr;
        Proto *handler = nullptr;

        /*
         * TODO:
         *
         * 1) EACH Proxy SHOULD BE ASSOCIATED WITH A JS-PEER:
         *    - CREATED DURING Proxy CONSTRUCTION:
         *      - ASSOCIATED WITH A "NAME" AND SOME "INDEX"
         *        - NAME COULD BE AUTOMATICALLY GENERATED (E.G. VIA "CLASS-NAME DEMANGLING")
         *          - BETTER ALTERNATIVE: A VIRTUAL string Proxy.peerName() METHOD
         *            - BECAUSE CLASS-NAME DEMANGLING IS NOT STANDARD
         *            - MORE FLEXIBLE ANYWAY
         *        - INDEX SHOULD BE AUTOMATICALLY INCREMENTED BASED ON INSTANCE-COUNT
         *        - E.G. SomeProxy proxy(); SomeProxy anotherProxy();
         *          - proxy WOULD BE NAMED SomeProxy, WITH AN INDEX OF 0
         *          - anotherProxy WOULD BE NAMED SomeProxy, WITH AN INDEX OF 1
         *    - GLOBALLY-ACCESSIBLE FROM THE JS-SIDE:
         *      - E.G. Peers.SomeProxy[0] (OR peers.SomeProxy[0]?)
         *      - THE GLOBAL Peers (OR peers?) OBJECT SHOULD BE MANAGED AT THE JS-COMPARTMENT LEVEL
         * 2) registerCallback(HandleObject object, ...) AND unregisterCallback(HandleObject object, ...)
         *    SHOULD NOT OPERATE ON SOME EXTERNAL JS-OBJECT BUT ON THE PROXY'S JS-PEER, E.G.
         *    - proxy.registerCallback("method1", BIND_STATIC_CALLBACK(staticMethod1));
         *      proxy.unregisterCallback("method1");
         *    - CONSIDER ADOPTING A "SIGNALS/SLOTS" SYNTAX, E.G.
         *      - proxy.registerSignal("signal1", BIND_STATIC_CALLBACK(staticMethod1));
         *      - proxy.unregisterSignal("signal1");
         * 3) THEN IT SHOULD BE POSSIBLE TO "CONNECT/DISCONNECT":
         *    - FROM THE C++ SIDE, E.G.
         *      - proxy.connect(globalHandle, "method1");
         *        proxy.disconnect(globalHandle, "method1");
         *    - OR FROM THE JS-SIDE, E.G.
         *      - var foo = {}; foo.method1 = peers.SomeProxy[0].method1; foo.method1(123);
         *        - peers.SomeProxy[0].method1 SHOULD BE A READ-ONLY PROPERTY
         */

        bool registerCallback(JS::HandleObject object, const std::string &name, const CallbackFn &fn);
        void unregisterCallback(JS::HandleObject object, const std::string &name);
        static bool dispatchCallback(JSContext *cx, unsigned argc, Value *vp);
        
    private:
        int32_t instanceId = -1;
        int32_t lastCallbackId = -1;
        std::map<int32_t, Callback> callbacks;
        
        void instanceCreated();
        void instanceDestroyed();
        
        Callback* getCallback(int32_t callbackId);
        int32_t getCallbackId(const std::string &name);
        int32_t addCallback(const std::string &name, const CallbackFn &fn);
        void removeCallback(int32_t callbackId);
    };
}

#undef TARGET
#undef HANDLE
#undef FORWARD
