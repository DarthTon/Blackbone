#include "Common.h"

namespace Testing
{
    TEST_CLASS( ScopeExit )
    {
    public:
        TEST_METHOD( Simple )
        {
            constexpr int expected = 10;
            int a = 0;
            {
                ON_SCOPE_EXIT{ a = expected; };
                AssertEx::AreEqual( 0, a );
            }

            AssertEx::AreEqual( expected, a );
        }
    };
}