/*
 * JSP: https://github.com/arielm/jsp
 * COPYRIGHT (C) 2014-2015, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE SIMPLIFIED BSD LICENSE:
 * https://github.com/arielm/jsp/blob/master/LICENSE
 */

#include "TestingRooting2.h"

#include "chronotext/Context.h"

using namespace std;
using namespace ci;
using namespace chr;

using namespace jsp;

void TestingRooting2::performSetup()
{
    WrappedValue::LOG_VERBOSE = false;
    WrappedObject::LOG_VERBOSE = false;
}

void TestingRooting2::performShutdown()
{
    WrappedValue::LOG_VERBOSE = false;
    WrappedObject::LOG_VERBOSE = false;
}

void TestingRooting2::performRun(bool force)
{
    if (force || true)
    {
        JSP_TEST(force || true, testAnalysis1)
        JSP_TEST(force || true, testAnalysis2)
        JSP_TEST(force || true, testAnalysis3)
    }
    
    if (force || true)
    {
        JSP_TEST(force || true, testBarkerJSFunctionality);
        JSP_TEST(force || true, testBarkerMixedFunctionality);
        JSP_TEST(force || true, testBarkerFinalization1)
    }
    
    if (force || true)
    {
        JSP_TEST(force || true, testRootedBarker1)
        JSP_TEST(force || true, testObjectAllocation1)
    }
    
    if (force || true)
    {
        JSP_TEST(force || true, testWrappedObjectAssignment1)
        JSP_TEST(force || true, testWrappedBarker1)
        JSP_TEST(force || true, testRootedWrappedBarker1)
        JSP_TEST(force || true, testHeapWrappedBarker1)
        JSP_TEST(force || true, testHeapWrappedJSBarker1)
    }
    
    if (force || true)
    {
        JSP_TEST(force || true, testBarkerPassedToJS1);
        JSP_TEST(force || true, testHeapWrappedJSBarker2);
    }
}

// ---

void TestingRooting2::testHandleObject1(HandleObject object)
{
    forceGC();
    JSP_CHECK(Barker::bark(object));
}

void TestingRooting2::testMutableHandleObject1(MutableHandleObject object)
{
    object.set(nullptr);
    forceGC();
}

// ---

static bool nativeCallback(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgsFromVp(argc, vp).rval().set(NumberValue(33));
    return true;
}

void TestingRooting2::testAnalysis1()
{
    JSObject *object = Barker::create("UNROOTED");
    
    JSP_CHECK(isInsideNursery(object));
    JSP_CHECK(writeGCDescriptor(object) == 'n');
    
    forceGC();
    
    JSP_CHECK(!isHealthy(object)); // REASON: object NOT ROOTED
    JSP_CHECK(writeGCDescriptor(object) == 'P');
}

void TestingRooting2::testAnalysis2()
{
    RootedObject object(cx, Barker::create("ROOTED"));
    JSFunction *function = JS_DefineFunction(cx, object, "someFunction", nativeCallback, 0, 0);
    
    LOGI << writeDetailed(function) << endl;
    JSP_CHECK(!isInsideNursery(function)); // XXX: IS function TENURED BECAUSE object IS ROOTED, OR IS IT ALWAYS THE CASE FOR NEW JSFunctions?
    
    forceGC();
    
    JSP_CHECK(isHealthy(function)); // REASON: function ROOTED (VIA object)
    JSP_CHECK(writeGCDescriptor(function) == 'B');

    JS_DeleteProperty(cx, object, "someFunction");
    forceGC();

    JSP_CHECK(!isHealthy(function)); // REASON: function NOT ROOTED ANYMORE
    JSP_CHECK(writeGCDescriptor(function) == 'P');
}

void TestingRooting2::testAnalysis3()
{
    JSString *s = toJSString("whatever");

    LOGI << writeDetailed(s) << endl;
    JSP_CHECK(!isInsideNursery(s)); // XXX: IT SEEMS THAT NEW JSStrings ARE ALWAYS TENURED

    forceGC();
    
    JSP_CHECK(!isHealthy(s)); // REASON: s NOT ROOTED
    JSP_CHECK(writeGCDescriptor(s) == 'P');
}

// ---

void TestingRooting2::testBarkerJSFunctionality()
{
    /*
     * FULL JS-SIDE FUNCTIONALITY:
     *
     * - CREATING UNROOTED BARKER
     * - ACCESSING id AND name PROPERTIES
     * - BARKING
     * - FORCING-GC (BARKER FINALIZED WHILE IN THE NURSERY)
     * - OBSERVING FINALIZATION
     */
    
    auto nextId = Barker::nextId();
    string name = "js-created unrooted 1";
    
    try
    {
        executeScript("var testBarkers1 = function(id, name) {\
                      var idMatch = (new Barker(name).id) == id;\
                      var nameMatch = Barker.getInstance(name).name == name;\
                      var barked = Barker.bark(name);\
                      forceGC();\
                      return idMatch && nameMatch && barked && Barker.isFinalized(name);\
                      }");
        
        AutoValueVector args(cx);
        args.append(toValue(nextId));
        args.append(toValue(name));
        
        JSP_CHECK(call(globalHandle(), "testBarkers1", args).isTrue());
    }
    catch (exception &e)
    {
        JSP_CHECK(false);
    }
}

void TestingRooting2::testBarkerMixedFunctionality()
{
    /*
     * MIXED FUNCTIONALITY:
     *
     * - C++ SIDE:
     *   - CREATING UNROOTED BARKER
     *   - ACCESSING id AND name
     *
     * - JS-SIDE:
     *   - BARKING
     *   - FORCING-GC (BARKER FINALIZED WHILE IN THE NURSERY)
     *   - OBSERVING FINALIZATION
     */

    auto nextId = Barker::nextId();
    string name = "CPP-CREATED UNROOTED 1";
    
    JSP_CHECK(Barker::getId(Barker::create(name)) == nextId);
    JSP_CHECK(Barker::getName(Barker::getInstance(name)) == name);
    
    try
    {
        executeScript("var testBarkers2 = function(name) {\
                      var barked = Barker.bark(name);\
                      forceGC();\
                      return barked && Barker.isFinalized(name);\
                      }");
        
        RootedValue arg(cx, toValue(name));
        JSP_CHECK(call(globalHandle(), "testBarkers2", arg).isTrue());
    }
    catch (exception &e)
    {
        JSP_CHECK(false);
    }
}

void TestingRooting2::testRootedBarker1()
{
    JSObject *barker = Barker::create("ASSIGNED 2");
    
    RootedObject rooted(cx, barker); // ASSIGNMENT 1
    
    /*
     * 1) WILL TRIGGER A GC
     * 2) WILL MAKE rooted BARK
     */
    testHandleObject1(rooted);
    
    /*
     * 1) ASSIGNMENT 2: WILL SET rooted TO NULL
     * 2) WILL TRIGGER A GC
     */
    testMutableHandleObject1(&rooted);
    
    JSP_CHECK(Barker::isFinalized("ASSIGNED 2"));
}

/*
 * TODO:
 *
 * 1) BRING-BACK THE POSSIBILITY TO ANALYSE OBJECTS DURING SPIDERMONKEY'S FINALIZATION-CALLBACK
 * 2) USE IT TO DEMONSTRATE THE "INTERESTING FACT" MENTIONED IN testBarkerFinalization1()
 * 3) USE IT TO SOLVE "MYSTERY" MENTIONED IN testHeapWrappedObject1()
 */

void TestingRooting2::testBarkerFinalization1()
{
    Barker::create("FINALIZATION 1");
    JSP_CHECK(isInsideNursery(Barker::getInstance("FINALIZATION 1"))); // CREATED IN THE NURSERY, AS INTENDED
    
    /*
     * INTERESTING FACT:
     *
     * WHEN AN OBJECT IS GARBAGE-COLLECTED WHILE IN THE NURSERY, SPIDERMONKEY
     * DOES NOT CONSIDER IT AS "ABOUT TO BE FINALIZED", AND SIMPLY MAKES IT POISONED
     *
     * THIS IS PROBABLY THE REASON WHY CLASSES WITH A FINALIZE-CALLBACK ARE
     * ALLOCATING OBJECTS DIRECTLY IN THE TENURED-HEAP
     */
    
    forceGC();
    JSP_CHECK(Barker::isFinalized("FINALIZATION 1"));
}

/*
 * TODO: THE FOLLOWING SHOULD BE FURTHER INVESTIGATED REGARDING "GENERIC JS-OBJECT CREATION"
 *
 * 1) THE NewObjectGCKind() METHOD DEFINED IN jsobj.cpp
 *    - IT SEEMS TO DEFINE A CREATED-OBJECT'S "FINALIZE" METHOD
 *      - E.G. gc::FINALIZE_OBJECT4
 *
 * 2) APPEARING IN THE (INSIGHTFUL NewObject() METHOD) DEFINED IN jsobj.cpp:
 *    gc::InitialHeap heap = GetInitialHeap(newKind, clasp);
 *
 * 3) SPIDERMONKEY'S "PROBES" MECHANISM (DEFINED /js/src/vm/Probes), NOTABLY:
 *    - bool CreateObject(ExclusiveContext *cx, JSObject *obj)
 *    - bool FinalizeObject(JSObject *obj)
 */

void TestingRooting2::testObjectAllocation1()
{
    /*
     * MYSTERY: OBJECT APPEARS TO BE ALLOCATED DIRECTLY IN THE TENURED-HEAP!?
     *
     *
     * FACTS:
     *
     * 1) IT DOESN'T MATTER IF {} OR "new Object({}) IS USED
     *
     * 2) THE "GENERIC JS-OBJECT" CLASS DOES NOT APPEAR TO HAVE A FINALIZE-CALLBACK
     *    - ASSUMING THIS IS THE CLASS USED HERE
     *
     * 3) A BARKER CREATED SIMILARELY VIA evaluateObject() WOULD BE ALLOCATED IN THE NURSERY
     *    - SEE testHeapWrappedJSBarker1()
     */
    
    JSObject *object = evaluateObject("({foo: 'baz', bar: 1.5})");
    JSP_CHECK(!isInsideNursery(object));
}

// ---

void TestingRooting2::testWrappedObjectAssignment1()
{
    JSObject *barkerA = Barker::create("ASSIGNED 1A");
    
    {
        WrappedObject wrapped; // ASSIGNMENT 1 (NO-OP)
        wrapped = Barker::create("ASSIGNED 1B"); // ASSIGNMENT 2
        wrapped = barkerA; // ASSIGNMENT 3
        
        Rooted<WrappedObject> rootedWrapped(cx, wrapped); // WILL PROTECT wrapped (AND THEREFORE barkerA) FROM GC
        
        forceGC();
        JSP_CHECK(Barker::bark(rootedWrapped.get())); // REASON: BARKER ROOTED
    }
    
    forceGC();
    JSP_CHECK(!Barker::isHealthy("ASSIGNED 1A")); // REASON: BARKER NOT STACK-ROOTED ANYMORE
}

void TestingRooting2::testWrappedBarker1()
{
    WrappedObject wrapped(Barker::create("WRAPPED 1"));
    JSP_CHECK(Barker::bark(wrapped));
    
    forceGC();
    JSP_CHECK(Barker::isFinalized("WRAPPED 1"));
}

void TestingRooting2::testRootedWrappedBarker1()
{
    JSObject *object = Barker::create("ROOTED-WRAPPED 1"); // CREATED IN THE NURSERY, AS INTENDED
    
    {
        Rooted<WrappedObject> rootedWrapped(cx, object);
        testHandleObject1(rootedWrapped);
    }
    
    forceGC();
    JSP_CHECK(Barker::isFinalized("ROOTED-WRAPPED 1"));
}

void TestingRooting2::testHeapWrappedBarker1()
{
    JSObject *object = Barker::create("HEAP-WRAPPED 1"); // CREATED IN THE NURSERY, AS INTENDED
    
    {
        Heap<WrappedObject> heapWrapped(object);
        testHandleObject1(heapWrapped); // AUTOMATIC-CONVERSION FROM Heap<WrappedObject> TO Handle<Object>
    }
    
    forceGC();
    JSP_CHECK(Barker::isFinalized("HEAP-WRAPPED 1"));
}

/*
 * EXAMPLE OF SOME "EXPECTED LOG" FOR testHeapWrappedBarker1
 *
 * TODO:
 *
 * 1) CONSIDER INTEGRATION IN THE FORTHCOMING "RECORDING / TESTING" SYSTEM:
 *    - INCLUDING FEATURES LIKE "SMART TEMPLATE VARIABLE ANALYSIS", ETC.
 *
 * 2) BEFOREHAND: SIMILAR SOLUTIONS SHOULD BE INVESTIGATED, E.G. /js/src/gdb/
 */

/*
testHeapWrappedBarker1
Barker CONSTRUCTED: 0x10c600000 {Barker b} [n] | HEAP-WRAPPED 1
jsp::WrappedObject::WrappedObject(JSObject *) 0x7fff5fbfb310 | object: 0x10c600000 {Barker b} [n]
jsp::WrappedObject::WrappedObject() 0x7fff5fbfb318 | object:
jsp::WrappedObject::WrappedObject(const jsp::WrappedObject &) 0x7fff5fbfb210 | object: 0x10c600000 {Barker b} [n]
void jsp::WrappedObject::operator=(const jsp::WrappedObject &) 0x7fff5fbfb318 | object: 0x10c600000 {Barker b} [n]
void jsp::WrappedObject::postBarrier() 0x7fff5fbfb318 | object: 0x10c600000 {Barker b} [n]
jsp::WrappedObject::~WrappedObject() 0x7fff5fbfb210 | object: 0x10c600000 {Barker b} [n]
jsp::WrappedObject::~WrappedObject() 0x7fff5fbfb310 | object: 0x10c600000 {Barker b} [n]
forceGC() | BEGIN
Barker TRACED: 0x10d743130 {Barker b} [W] | HEAP-WRAPPED 1
void jsp::WrappedObject::trace(JSTracer *) 0x7fff5fbfb318 | object: 0x10d743130 {Barker b} [B]
Barker TRACED: 0x10d743130 {Barker b} [B] | HEAP-WRAPPED 1
forceGC() | END
Barker BARKED: 0x10d743130 {Barker b} [B] | HEAP-WRAPPED 1
void jsp::WrappedObject::relocate() 0x7fff5fbfb318 | object: 0x10d743130 {Barker b} [B]
jsp::WrappedObject::~WrappedObject() 0x7fff5fbfb318 | object: 0x10d743130 {Barker b} [B]
forceGC() | BEGIN
Barker FINALIZED: 0x10d743130 [P] | HEAP-WRAPPED 1
forceGC() | END
*/

void TestingRooting2::testHeapWrappedJSBarker1()
{
    JSObject *object = evaluateObject("new Barker('heap-wrapped-js 1')");
    JSP_CHECK(isInsideNursery(object)); // CREATED IN THE NURSERY, AS INTENDED
    
    {
        Heap<WrappedObject> heapWrapped(object);
        
        /*
         * GC WILL MOVE THE BARKER TO THE TENURED-HEAP, TURNING object INTO A DANGLING POINTER
         *
         * DURING GC:
         *
         * 1) THE BARKER'S trace CALLBACK IS INVOKED BY THE SYSTEM:
         *    - NOTIFYING THAT THE BARKER WAS MOVED THE TENURED-HEAP
         *      - AS EXPLAINED IN HeapAPI.h AND GCAPI.h:
         *        "All live objects in the nursery are moved to tenured at the beginning of each GC slice"
         *    - AT THIS STAGE, THE BARKER IS "WHITE":
         *      - AS EXPLAINED IN: https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/Internals/Garbage_collection
         *        - BLACK: "In common CS terminology, an object is black during the mark phase if it has been marked and its children
         *                  are gray (have been queued for marking). An object is black after the mark phase if it has been marked.
         *                  In SpiderMonkey, an object is black if its mark bit is set."
         *        - GRAY:  "In common CS terminology, an object is gray during the mark phase if it has been queued for marking.
         *                  In SpiderMonkey, an object is gray if it is a child of an object in the mark stack and it is not black.
         *                  Thus, gray objects are not represented explictly."
         *        - WHITE: "In common CS terminology, an object is white during the mark phase if it has not been seen yet.
         *                  An object is white after the mark phase if it has not been marked. In SpiderMonkey, an object is white if it is not gray or black;
         *                  i.e., it is not black and it is not a child of an object in the mark stack."
         *
         * 2) WrappedObject::trace IS INVOKED:
         *    - IN TURN, IT WILL CAUSE Barker::trace TO BE RE-INVOKED
         *      - AT THIS STAGE, THE BARKED WILL BE "BLACK"
         *    - WHY IS WrappedObject::trace INVOKED AT THE FIRST PLACE?
         *      - BECAUSE THE WrappedObject REGISTERED ITSELF TO OUR "CENTRALIZED EXTRA-ROOT-TRACING" SYSTEM
         *        DURING WrappedObject::postBarrier(), WHICH IS AUTOMATICALLY CALLED WHILE ENCLOSED IN A Heap<WrappedObject>
         */
        
        forceGC();
        
        JSP_CHECK(!isHealthy(object)); // ACCESSING THE BARKER VIA object WOULD BE A GC-HAZARD
        JSP_CHECK(Barker::bark(heapWrapped.get())); // PASSING THROUGH Heap<WrappedObject> LEADS TO THE MOVED BARKER
    }
    
    forceGC();
    JSP_CHECK(Barker::isFinalized("heap-wrapped-js 1"));
}

// ---

/*
 * IT IS NECESSARY TO USE ROOTED VALUES WHEN PASSING ARGUMENTS TO A JS-FUNCTION CALLED FROM C++,
 * WHICH IN TURNS AFFECTS THE CAPABILITY TO TEST NON-ROOTED OBJECTS FROM ONE "SIDE" (I.E. JS OR C++) TO THE OTHER
 *
 * ANOTHER APPROACH MUST THEREFORE BE USED, AS DEMONSTRATED IN THE FOLLOWING:
 * - testBarkerJSFunctionality()
 * - testBarkerMixedFunctionality()
 */

void TestingRooting2::testBarkerPassedToJS1()
{
    executeScript("function handleBarker1(barker) { barker.bark(); }");

    {
        RootedValue arg(cx, Barker::create("PASSED-TO-JS 1"));
        call(globalHandle(), "handleBarker1", arg);
    }
    
    forceGC();
    JSP_CHECK(Barker::isFinalized("PASSED-TO-JS 1")); // REASON: BARKER NOT ROOTED ANYMORE
}

void TestingRooting2::testHeapWrappedJSBarker2()
{
    executeScript("new Barker('HEAP-WRAPPED 2')"); // CREATED IN THE NURSERY, AS INTENDED

    {
        Heap<WrappedValue> heapWrapped(Barker::getInstance("HEAP-WRAPPED 2"));
        
        forceGC();
        JSP_CHECK(Barker::bark(heapWrapped.get()));
    }
    
    forceGC();
    JSP_CHECK(Barker::isFinalized("HEAP-WRAPPED 2"));
}
