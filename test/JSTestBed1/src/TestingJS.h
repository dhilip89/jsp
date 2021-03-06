/*
 * JSP: https://github.com/arielm/jsp
 * COPYRIGHT (C) 2014-2015, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE SIMPLIFIED BSD LICENSE:
 * https://github.com/arielm/jsp/blob/master/LICENSE
 */

#pragma once

#include "TestingJSBase.h"

class TestingJS : public TestingJSBase
{
public:
    void performRun(bool force = false) final;
    
    // ---

    void testShapes1();
    void testShapes2();
    void testAtoms();
    void testReservedSlots();
    void testJSID();

    void dumpIds(JSObject *object);
    
    JSObject* createObject1();
    JSObject* createObject2();
    JSObject* createArray();

    // ---
    
    void testThreadSafety();
    
    void testEvaluationScope();
    void testFunctionScope();
    void testCustomScriptExecution();
    
    // ---
    
    void testParsing1();
    void testParsing2();
    void initComplexJSON(const std::string &source);

    void testStringify();
    void testToSource();
    void initComplexJSObject();
    
    // ---
    
    void testGetter1();
    void testSetter1();
    void testGetterSetter1();
    
    // ---
    
    static const JSClass CustomClass1;
    static const JSClass CustomClass2;
    static bool CustomConstructor(JSContext *cx, unsigned argc, JS::Value *vp);

    void testCustomConstruction1();
    void testCustomConstruction2();
    void testNewObject();
    
    // ---

    void testReadOnlyProperty1();
    void testReadOnlyProperty2();

    void testPermanentProperty1();
    void testPermanentProperty2();
    
    void testObjectReset1();
    void testObjectReset2();
    
    // ---
    
    void testGetProperty1();
    void testGetElement1();

    void testGetProperties1();
    void testGetProperties2();
    void testProperties(JS::HandleObject object, const std::vector<std::string> &properties, JS::HandleObject o, const std::string &s1, const char *s2);
    
    void testGetElements1();
    
    // ---

    void testToValue1();
    void testToValue2();
    void setValues1(JS::HandleObject array, const std::vector<int> &indices, JS::HandleObject o, const std::string &s1, const char *s2);

    void testSetElements1();
    void testSetElements2();
    void setValues2(JS::HandleObject array, const std::vector<int> &indices, JS::HandleObject o, const std::string &s1, const char *s2);
    
    // ---
    
    void testBulkArrayOperations1();
    void testBulkArrayOperations2();
    
    void testBulkArrayOperations3();
    void testBulkArrayOperations4();
    
    // ---
    
    void testArrayElementCount();
};
