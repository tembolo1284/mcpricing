/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-2021 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#ifndef UNITY_FRAMEWORK_H
#define UNITY_FRAMEWORK_H
#define UNITY

#include <stdint.h>
#include <stddef.h>

/*-------------------------------------------------------
 * Configuration Options
 *-------------------------------------------------------*/

#ifndef UNITY_EXCLUDE_FLOAT
#define UNITY_INCLUDE_FLOAT
#endif

#ifndef UNITY_EXCLUDE_DOUBLE
#define UNITY_INCLUDE_DOUBLE
#endif

#ifndef UNITY_OUTPUT_CHAR
#include <stdio.h>
#define UNITY_OUTPUT_CHAR(a) (void)putchar(a)
#endif

#ifndef UNITY_OUTPUT_FLUSH
#define UNITY_OUTPUT_FLUSH() (void)fflush(stdout)
#endif

#ifndef UNITY_PRINT_EOL
#define UNITY_PRINT_EOL() UNITY_OUTPUT_CHAR('\n')
#endif

/*-------------------------------------------------------
 * Internal Types
 *-------------------------------------------------------*/

typedef int64_t  UNITY_INT;
typedef uint64_t UNITY_UINT;
typedef double   UNITY_DOUBLE;
typedef float    UNITY_FLOAT;

typedef enum {
    UNITY_DISPLAY_STYLE_INT = 0,
    UNITY_DISPLAY_STYLE_UINT,
    UNITY_DISPLAY_STYLE_HEX8,
    UNITY_DISPLAY_STYLE_HEX16,
    UNITY_DISPLAY_STYLE_HEX32,
    UNITY_DISPLAY_STYLE_HEX64
} UNITY_DISPLAY_STYLE_T;

struct UNITY_STORAGE_T {
    const char* TestFile;
    const char* CurrentTestName;
    UNITY_UINT  CurrentTestLineNumber;
    UNITY_UINT  NumberOfTests;
    UNITY_UINT  TestFailures;
    UNITY_UINT  TestIgnores;
    UNITY_UINT  CurrentTestFailed;
    UNITY_UINT  CurrentTestIgnored;
};

extern struct UNITY_STORAGE_T Unity;

/*-------------------------------------------------------
 * Test Execution
 *-------------------------------------------------------*/

void UnityBegin(const char* filename);
int  UnityEnd(void);
void UnityDefaultTestRun(void (*func)(void), const char* name, int line);

/*-------------------------------------------------------
 * Test Definition Macros
 *-------------------------------------------------------*/

#define TEST_PROTECT() 1
#define TEST_ABORT() return

#define RUN_TEST(func) UnityDefaultTestRun(func, #func, __LINE__)

#define TEST_LINE_NUM (Unity.CurrentTestLineNumber)
#define TEST_IS_IGNORED (Unity.CurrentTestIgnored)

/*-------------------------------------------------------
 * Basic Assertions
 *-------------------------------------------------------*/

void UnityAssertBits(const UNITY_INT mask,
                     const UNITY_INT expected,
                     const UNITY_INT actual,
                     const char* msg,
                     const unsigned int lineNumber);

void UnityAssertEqualNumber(const UNITY_INT expected,
                            const UNITY_INT actual,
                            const char* msg,
                            const unsigned int lineNumber,
                            const UNITY_DISPLAY_STYLE_T style);

void UnityAssertGreaterOrLessOrEqualNumber(const UNITY_INT threshold,
                                           const UNITY_INT actual,
                                           const int compare,
                                           const char* msg,
                                           const unsigned int lineNumber,
                                           const UNITY_DISPLAY_STYLE_T style);

void UnityAssertEqualString(const char* expected,
                            const char* actual,
                            const char* msg,
                            const unsigned int lineNumber);

void UnityAssertEqualMemory(const void* expected,
                            const void* actual,
                            const UNITY_UINT length,
                            const UNITY_UINT num_elements,
                            const char* msg,
                            const unsigned int lineNumber);

void UnityAssertNumbersWithin(const UNITY_UINT delta,
                              const UNITY_INT expected,
                              const UNITY_INT actual,
                              const char* msg,
                              const unsigned int lineNumber,
                              const UNITY_DISPLAY_STYLE_T style);

void UnityFail(const char* message, const unsigned int line);
void UnityIgnore(const char* message, const unsigned int line);
void UnityMessage(const char* message, const unsigned int line);

/*-------------------------------------------------------
 * Floating Point Assertions
 *-------------------------------------------------------*/

#ifdef UNITY_INCLUDE_FLOAT
void UnityAssertFloatsWithin(const UNITY_FLOAT delta,
                             const UNITY_FLOAT expected,
                             const UNITY_FLOAT actual,
                             const char* msg,
                             const unsigned int lineNumber);

void UnityAssertFloatSpecial(const UNITY_FLOAT actual,
                             const char* msg,
                             const unsigned int lineNumber,
                             const int style);
#endif

#ifdef UNITY_INCLUDE_DOUBLE
void UnityAssertDoublesWithin(const UNITY_DOUBLE delta,
                              const UNITY_DOUBLE expected,
                              const UNITY_DOUBLE actual,
                              const char* msg,
                              const unsigned int lineNumber);

void UnityAssertDoubleSpecial(const UNITY_DOUBLE actual,
                              const char* msg,
                              const unsigned int lineNumber,
                              const int style);
#endif

/*-------------------------------------------------------
 * Test Assert Macros
 *-------------------------------------------------------*/

#define UNITY_TEST_FAIL(line, message)    UnityFail((message), (unsigned int)(line))
#define UNITY_TEST_IGNORE(line, message)  UnityIgnore((message), (unsigned int)(line))

/* Boolean */
#define TEST_ASSERT(condition) \
    do { if (!(condition)) { UNITY_TEST_FAIL(__LINE__, "Expression was FALSE"); } } while(0)

#define TEST_ASSERT_TRUE(condition)  TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))
#define TEST_ASSERT_NULL(pointer)    TEST_ASSERT((pointer) == NULL)
#define TEST_ASSERT_NOT_NULL(pointer) TEST_ASSERT((pointer) != NULL)

#define TEST_FAIL()               UNITY_TEST_FAIL(__LINE__, NULL)
#define TEST_FAIL_MESSAGE(msg)    UNITY_TEST_FAIL(__LINE__, (msg))
#define TEST_IGNORE()             UNITY_TEST_IGNORE(__LINE__, NULL)
#define TEST_IGNORE_MESSAGE(msg)  UNITY_TEST_IGNORE(__LINE__, (msg))
#define TEST_MESSAGE(msg)         UnityMessage((msg), __LINE__)

/* Integer Equality */
#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_EQUAL_INT_MESSAGE(expected, actual, msg) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), (msg), __LINE__, UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_EQUAL_UINT(expected, actual) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_UINT)

#define TEST_ASSERT_EQUAL_UINT64(expected, actual) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_UINT)

#define TEST_ASSERT_EQUAL(expected, actual) \
    TEST_ASSERT_EQUAL_INT((expected), (actual))

/* Integer Comparison */
#define TEST_ASSERT_GREATER_THAN(threshold, actual) \
    UnityAssertGreaterOrLessOrEqualNumber((UNITY_INT)(threshold), (UNITY_INT)(actual), 1, NULL, __LINE__, UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_LESS_THAN(threshold, actual) \
    UnityAssertGreaterOrLessOrEqualNumber((UNITY_INT)(threshold), (UNITY_INT)(actual), -1, NULL, __LINE__, UNITY_DISPLAY_STYLE_INT)

/* Integer Within Range */
#define TEST_ASSERT_INT_WITHIN(delta, expected, actual) \
    UnityAssertNumbersWithin((delta), (UNITY_INT)(expected), (UNITY_INT)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_INT)

/* Hex */
#define TEST_ASSERT_EQUAL_HEX(expected, actual) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_HEX32)

#define TEST_ASSERT_EQUAL_HEX64(expected, actual) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_HEX64)

/* String */
#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    UnityAssertEqualString((expected), (actual), NULL, __LINE__)

#define TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, msg) \
    UnityAssertEqualString((expected), (actual), (msg), __LINE__)

/* Memory */
#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) \
    UnityAssertEqualMemory((expected), (actual), (len), 1, NULL, __LINE__)

/* Float */
#ifdef UNITY_INCLUDE_FLOAT
#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) \
    UnityAssertFloatsWithin((UNITY_FLOAT)(delta), (UNITY_FLOAT)(expected), (UNITY_FLOAT)(actual), NULL, __LINE__)

#define TEST_ASSERT_FLOAT_WITHIN_MESSAGE(delta, expected, actual, msg) \
    UnityAssertFloatsWithin((UNITY_FLOAT)(delta), (UNITY_FLOAT)(expected), (UNITY_FLOAT)(actual), (msg), __LINE__)

#define TEST_ASSERT_EQUAL_FLOAT(expected, actual) \
    UnityAssertFloatsWithin((UNITY_FLOAT)0.00001f, (UNITY_FLOAT)(expected), (UNITY_FLOAT)(actual), NULL, __LINE__)
#endif

/* Double */
#ifdef UNITY_INCLUDE_DOUBLE
#define TEST_ASSERT_DOUBLE_WITHIN(delta, expected, actual) \
    UnityAssertDoublesWithin((UNITY_DOUBLE)(delta), (UNITY_DOUBLE)(expected), (UNITY_DOUBLE)(actual), NULL, __LINE__)

#define TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(delta, expected, actual, msg) \
    UnityAssertDoublesWithin((UNITY_DOUBLE)(delta), (UNITY_DOUBLE)(expected), (UNITY_DOUBLE)(actual), (msg), __LINE__)

#define TEST_ASSERT_EQUAL_DOUBLE(expected, actual) \
    UnityAssertDoublesWithin((UNITY_DOUBLE)0.00001, (UNITY_DOUBLE)(expected), (UNITY_DOUBLE)(actual), NULL, __LINE__)
#endif

/* Pointer */
#define TEST_ASSERT_EQUAL_PTR(expected, actual) \
    UnityAssertEqualNumber((UNITY_INT)(expected), (UNITY_INT)(actual), NULL, __LINE__, UNITY_DISPLAY_STYLE_HEX64)

#endif /* UNITY_FRAMEWORK_H */
