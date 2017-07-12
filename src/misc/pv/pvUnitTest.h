/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef PVUNITTEST_H
#define PVUNITTEST_H

#include <sstream>
#include <typeinfo>

#include <epicsUnitTest.h>

#include <pv/sharedPtr.h>
#include <pv/epicsException.h>
#include <pv/pvData.h>

namespace detail {

template<class C, void (C::*M)()>
void test_method(const char *kname, const char *mname)
{
    try {
        testDiag("------- %s::%s --------", kname, mname);
        C inst;
        (inst.*M)();
    } catch(std::exception& e) {
        PRINT_EXCEPTION(e);
        testAbort("unexpected exception: %s", e.what());
    }
}

class testPassx
{
    std::ostringstream strm;
    bool pass, alive;
public:
    explicit testPassx(bool r) :pass(r), alive(true) {}
    ~testPassx() {
        if(alive)
            testOk(pass, "%s", strm.str().c_str());
    }
    template<typename T>
    inline testPassx& operator<<(T v) {
        strm<<v;
        return *this;
    }

    // move ctor masquerading as copy ctor
    testPassx(testPassx& o) :strm(o.strm.str()), pass(o.pass), alive(o.alive) { strm.seekp(0, std::ios_base::end); o.alive = false; }
private:
    testPassx& operator=(const testPassx&);
};

template<typename LHS, typename RHS>
inline testPassx testEqualx(const char *nLHS, const char *nRHS, LHS l, RHS r)
{
    return testPassx(l==r)<<nLHS<<" ("<<l<<") == "<<nRHS<<" ("<<r<<")";
}

}//namespace detail

/** @defgroup testhelpers Unit testing helpers
 *
 * Helper functions for writing unit tests.
 *
 @include unittest.cpp
 *
 * @{
 */

/** Run a class method as a test.
 *
 * Each invocation of TEST_METHOD() constructs a new instance of 'klass' on the stack.
 * Thus constructor and destructor can be used for common test setup and tear down.
 @code
 struct MyTest {
   MyTest() { } // setup
   ~MyTest() { } // tear down
   void test1() {}
   void test2() {}
 };
 MAIN(somename) {
   testPlan(0);
   TEST_METHOD(MyTest, test1)
   TEST_METHOD(MyTest, test2)
   return testDone();
 }
 @endcode
 */
#define TEST_METHOD(klass, method) ::detail::test_method<klass, &klass::method>(#klass, #method)

/** Compare equality.  print left and right hand values and expression strings
 *
 @code
 int x=5;
 testEqual(x, 5);
 // prints "ok 1 - x (5) == 5 (5)\n"
 testEqual(x, 6)<<" oops";
 // prints "not ok 1 - x (5) == 6 (6) oops\n"
 @endcode
 */
#define testEqual(LHS, RHS) ::detail::testEqualx(#LHS, #RHS, LHS, RHS)

/** Pass/fail from boolean
 *
 @code
 bool y=true;
 testTrue(y);
 // prints "ok 1 - y\n"
 testTrue(!y)<<" oops";
 // prints "not ok 1 - !y oops\n"
 @endcode
 */
#define testTrue(B) ::detail::testPassx(!!(B))<<#B

/** Compare value of PVStructure field
 *
 @code
 PVStructurePtr x(.....);
 testFieldEqual<epics::pvData::PVInt>(x, "alarm.severity", 1);
 @endcode
 */
template<typename PVD>
::detail::testPassx
testFieldEqual(const std::tr1::shared_ptr<epics::pvData::PVStructure>& val, const char *name, typename PVD::value_type expect)
{
    if(!val) {
        return ::detail::testPassx(false)<<" null structure pointer";
    }
    typename PVD::shared_pointer fval(val->getSubField<PVD>(name));
    if(!fval) {
        return ::detail::testPassx(false)<<" field '"<<name<<"' with type "<<typeid(PVD).name()<<" does not exist";
    } else {
        typename PVD::value_type actual(fval->get());
        return ::detail::testPassx(actual==expect)<<name<<" ("<<actual<<") == "<<expect;
    }
}

template<typename PVD>
::detail::testPassx
testFieldEqual(const std::tr1::shared_ptr<epics::pvData::PVStructure>& val, const char *name, typename PVD::const_svector expect)
{
    if(!val) {
        return ::detail::testPassx(false)<<" null structure pointer";
    }
    typename PVD::shared_pointer fval(val->getSubField<PVD>(name));
    if(!fval) {
        return ::detail::testPassx(false)<<" field '"<<name<<"' with type "<<typeid(PVD).name()<<" does not exist";
    } else {
        typename PVD::const_svector actual(fval->view());
        return ::detail::testPassx(actual==expect)<<name<<" ("<<actual<<") == "<<expect;
    }
}

/** @} */

#endif // PVUNITTEST_H