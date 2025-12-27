/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-2021 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include "unity.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/*-------------------------------------------------------
 * Global State
 *-------------------------------------------------------*/

struct UNITY_STORAGE_T Unity;

static const char UnityStrOk[]     = "OK";
static const char UnityStrPass[]   = "PASS";
static const char UnityStrFail[]   = "FAIL";
static const char UnityStrIgnore[] = "IGNORE";

/*-------------------------------------------------------
 * Output Helpers
 *-------------------------------------------------------*/

static void UnityPrintChar(const char c)
{
    UNITY_OUTPUT_CHAR(c);
}

static void UnityPrint(const char* str)
{
    if (str != NULL) {
        while (*str) {
            UnityPrintChar(*str);
            str++;
        }
    }
}

static void UnityPrintNumber(const UNITY_INT number)
{
    char buffer[32];
    UNITY_INT n = number;
    int i = 0;
    int neg = 0;

    if (n < 0) {
        neg = 1;
        n = -n;
    }

    do {
        buffer[i++] = (char)('0' + (n % 10));
        n /= 10;
    } while (n > 0);

    if (neg) {
        UnityPrintChar('-');
    }

    while (i > 0) {
        UnityPrintChar(buffer[--i]);
    }
}

static void UnityPrintNumberUnsigned(const UNITY_UINT number)
{
    char buffer[32];
    UNITY_UINT n = number;
    int i = 0;

    do {
        buffer[i++] = (char)('0' + (n % 10));
        n /= 10;
    } while (n > 0);

    while (i > 0) {
        UnityPrintChar(buffer[--i]);
    }
}

static void UnityPrintNumberHex(const UNITY_UINT number, const int width)
{
    static const char hex[] = "0123456789ABCDEF";
    char buffer[16];
    int i;

    for (i = 0; i < width; i++) {
        buffer[i] = hex[(number >> (4 * (width - 1 - i))) & 0xF];
    }

    UnityPrint("0x");
    for (i = 0; i < width; i++) {
        UnityPrintChar(buffer[i]);
    }
}

#ifdef UNITY_INCLUDE_DOUBLE
static void UnityPrintFloat(const UNITY_DOUBLE number)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.6f", number);
    UnityPrint(buffer);
}
#endif

/*-------------------------------------------------------
 * Test Execution
 *-------------------------------------------------------*/

void UnityBegin(const char* filename)
{
    Unity.TestFile = filename;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;

    UnityPrint("-------------------------------------------\n");
    UnityPrint("Running tests in: ");
    UnityPrint(filename);
    UnityPrint("\n-------------------------------------------\n");
}

int UnityEnd(void)
{
    UnityPrint("\n-------------------------------------------\n");
    UnityPrintNumber((UNITY_INT)Unity.NumberOfTests);
    UnityPrint(" Tests ");
    UnityPrintNumber((UNITY_INT)Unity.TestFailures);
    UnityPrint(" Failures ");
    UnityPrintNumber((UNITY_INT)Unity.TestIgnores);
    UnityPrint(" Ignored\n");

    if (Unity.TestFailures == 0) {
        UnityPrint(UnityStrOk);
    } else {
        UnityPrint(UnityStrFail);
    }
    UnityPrint("\n");
    UNITY_OUTPUT_FLUSH();

    return (int)Unity.TestFailures;
}

void UnityDefaultTestRun(void (*func)(void), const char* name, int line)
{
    Unity.CurrentTestName = name;
    Unity.CurrentTestLineNumber = (UNITY_UINT)line;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
    Unity.NumberOfTests++;

    UnityPrint(name);
    UnityPrint("...");

    func();

    if (Unity.CurrentTestIgnored) {
        UnityPrint(UnityStrIgnore);
    } else if (Unity.CurrentTestFailed) {
        UnityPrint(UnityStrFail);
    } else {
        UnityPrint(UnityStrPass);
    }
    UnityPrint("\n");
    UNITY_OUTPUT_FLUSH();
}

/*-------------------------------------------------------
 * Assertion Implementations
 *-------------------------------------------------------*/

void UnityFail(const char* message, const unsigned int line)
{
    Unity.CurrentTestFailed = 1;
    Unity.TestFailures++;

    UnityPrint("\n  FAILED at line ");
    UnityPrintNumber((UNITY_INT)line);
    if (message != NULL) {
        UnityPrint(": ");
        UnityPrint(message);
    }
    UnityPrint("\n");
}

void UnityIgnore(const char* message, const unsigned int line)
{
    Unity.CurrentTestIgnored = 1;
    Unity.TestIgnores++;

    UnityPrint("\n  IGNORED at line ");
    UnityPrintNumber((UNITY_INT)line);
    if (message != NULL) {
        UnityPrint(": ");
        UnityPrint(message);
    }
    UnityPrint("\n");
}

void UnityMessage(const char* message, const unsigned int line)
{
    UnityPrint("\n  INFO at line ");
    UnityPrintNumber((UNITY_INT)line);
    UnityPrint(": ");
    UnityPrint(message);
    UnityPrint("\n");
}

void UnityAssertEqualNumber(const UNITY_INT expected,
                            const UNITY_INT actual,
                            const char* msg,
                            const unsigned int lineNumber,
                            const UNITY_DISPLAY_STYLE_T style)
{
    if (expected != actual) {
        Unity.CurrentTestFailed = 1;
        Unity.TestFailures++;

        UnityPrint("\n  FAILED at line ");
        UnityPrintNumber((UNITY_INT)lineNumber);
        UnityPrint("\n    Expected: ");

        if (style == UNITY_DISPLAY_STYLE_HEX32) {
            UnityPrintNumberHex((UNITY_UINT)expected, 8);
        } else if (style == UNITY_DISPLAY_STYLE_HEX64) {
            UnityPrintNumberHex((UNITY_UINT)expected, 16);
        } else if (style == UNITY_DISPLAY_STYLE_UINT) {
            UnityPrintNumberUnsigned((UNITY_UINT)expected);
        } else {
            UnityPrintNumber(expected);
        }

        UnityPrint("\n    Actual:   ");

        if (style == UNITY_DISPLAY_STYLE_HEX32) {
            UnityPrintNumberHex((UNITY_UINT)actual, 8);
        } else if (style == UNITY_DISPLAY_STYLE_HEX64) {
            UnityPrintNumberHex((UNITY_UINT)actual, 16);
        } else if (style == UNITY_DISPLAY_STYLE_UINT) {
            UnityPrintNumberUnsigned((UNITY_UINT)actual);
        } else {
            UnityPrintNumber(actual);
        }

        if (msg != NULL) {
            UnityPrint("\n    Message:  ");
            UnityPrint(msg);
        }
        UnityPrint("\n");
    }
}

void UnityAssertGreaterOrLessOrEqualNumber(const UNITY_INT threshold,
                                           const UNITY_INT actual,
                                           const int compare,
                                           const char* msg,
                                           const unsigned int lineNumber,
                                           const UNITY_DISPLAY_STYLE_T style)
{
    int failed = 0;
    (void)style;  /* Unused for now */

    if (compare > 0) {  /* Greater than */
        if (actual <= threshold) failed = 1;
    } else if (compare < 0) {  /* Less than */
        if (actual >= threshold) failed = 1;
    } else {  /* Equal */
        if (actual != threshold) failed = 1;
    }

    if (failed) {
        Unity.CurrentTestFailed = 1;
        Unity.TestFailures++;

        UnityPrint("\n  FAILED at line ");
        UnityPrintNumber((UNITY_INT)lineNumber);
        UnityPrint("\n    Threshold: ");
        UnityPrintNumber(threshold);
        UnityPrint("\n    Actual:    ");
        UnityPrintNumber(actual);
        if (msg != NULL) {
            UnityPrint("\n    Message:   ");
            UnityPrint(msg);
        }
        UnityPrint("\n");
    }
}

void UnityAssertNumbersWithin(const UNITY_UINT delta,
                              const UNITY_INT expected,
                              const UNITY_INT actual,
                              const char* msg,
                              const unsigned int lineNumber,
                              const UNITY_DISPLAY_STYLE_T style)
{
    UNITY_INT diff = actual - expected;
    (void)style;  /* Unused for now */
    if (diff < 0) diff = -diff;

    if ((UNITY_UINT)diff > delta) {
        Unity.CurrentTestFailed = 1;
        Unity.TestFailures++;

        UnityPrint("\n  FAILED at line ");
        UnityPrintNumber((UNITY_INT)lineNumber);
        UnityPrint("\n    Expected: ");
        UnityPrintNumber(expected);
        UnityPrint(" +/- ");
        UnityPrintNumberUnsigned(delta);
        UnityPrint("\n    Actual:   ");
        UnityPrintNumber(actual);
        if (msg != NULL) {
            UnityPrint("\n    Message:  ");
            UnityPrint(msg);
        }
        UnityPrint("\n");
    }
}

void UnityAssertEqualString(const char* expected,
                            const char* actual,
                            const char* msg,
                            const unsigned int lineNumber)
{
    int failed = 0;

    if (expected == NULL && actual == NULL) {
        return;  /* Both NULL - equal */
    }

    if (expected == NULL || actual == NULL) {
        failed = 1;
    } else if (strcmp(expected, actual) != 0) {
        failed = 1;
    }

    if (failed) {
        Unity.CurrentTestFailed = 1;
        Unity.TestFailures++;

        UnityPrint("\n  FAILED at line ");
        UnityPrintNumber((UNITY_INT)lineNumber);
        UnityPrint("\n    Expected: \"");
        UnityPrint(expected ? expected : "(null)");
        UnityPrint("\"\n    Actual:   \"");
        UnityPrint(actual ? actual : "(null)");
        UnityPrint("\"");
        if (msg != NULL) {
            UnityPrint("\n    Message:  ");
            UnityPrint(msg);
        }
        UnityPrint("\n");
    }
}

void UnityAssertEqualMemory(const void* expected,
                            const void* actual,
                            const UNITY_UINT length,
                            const UNITY_UINT num_elements,
                            const char* msg,
                            const unsigned int lineNumber)
{
    const unsigned char* exp = (const unsigned char*)expected;
    const unsigned char* act = (const unsigned char*)actual;
    UNITY_UINT i;

    if (expected == NULL && actual == NULL) {
        return;
    }

    if (expected == NULL || actual == NULL) {
        UnityFail(msg ? msg : "Memory comparison with NULL pointer", lineNumber);
        return;
    }

    for (i = 0; i < length * num_elements; i++) {
        if (exp[i] != act[i]) {
            Unity.CurrentTestFailed = 1;
            Unity.TestFailures++;

            UnityPrint("\n  FAILED at line ");
            UnityPrintNumber((UNITY_INT)lineNumber);
            UnityPrint("\n    Memory mismatch at byte ");
            UnityPrintNumberUnsigned(i);
            UnityPrint("\n    Expected: 0x");
            UnityPrintNumberHex(exp[i], 2);
            UnityPrint("\n    Actual:   0x");
            UnityPrintNumberHex(act[i], 2);
            if (msg != NULL) {
                UnityPrint("\n    Message:  ");
                UnityPrint(msg);
            }
            UnityPrint("\n");
            return;
        }
    }
}

/*-------------------------------------------------------
 * Floating Point Assertions
 *-------------------------------------------------------*/

#ifdef UNITY_INCLUDE_FLOAT
void UnityAssertFloatsWithin(const UNITY_FLOAT delta,
                             const UNITY_FLOAT expected,
                             const UNITY_FLOAT actual,
                             const char* msg,
                             const unsigned int lineNumber)
{
    UNITY_FLOAT diff = actual - expected;
    if (diff < 0) diff = -diff;

    if (diff > delta) {
        Unity.CurrentTestFailed = 1;
        Unity.TestFailures++;

        UnityPrint("\n  FAILED at line ");
        UnityPrintNumber((UNITY_INT)lineNumber);
        UnityPrint("\n    Expected: ");
        UnityPrintFloat((UNITY_DOUBLE)expected);
        UnityPrint(" +/- ");
        UnityPrintFloat((UNITY_DOUBLE)delta);
        UnityPrint("\n    Actual:   ");
        UnityPrintFloat((UNITY_DOUBLE)actual);
        if (msg != NULL) {
            UnityPrint("\n    Message:  ");
            UnityPrint(msg);
        }
        UnityPrint("\n");
    }
}

void UnityAssertFloatSpecial(const UNITY_FLOAT actual,
                             const char* msg,
                             const unsigned int lineNumber,
                             const int style)
{
    /* Placeholder for NaN, Inf checks */
    (void)actual;
    (void)msg;
    (void)lineNumber;
    (void)style;
}
#endif

#ifdef UNITY_INCLUDE_DOUBLE
void UnityAssertDoublesWithin(const UNITY_DOUBLE delta,
                              const UNITY_DOUBLE expected,
                              const UNITY_DOUBLE actual,
                              const char* msg,
                              const unsigned int lineNumber)
{
    UNITY_DOUBLE diff = actual - expected;
    if (diff < 0) diff = -diff;

    if (diff > delta) {
        Unity.CurrentTestFailed = 1;
        Unity.TestFailures++;

        UnityPrint("\n  FAILED at line ");
        UnityPrintNumber((UNITY_INT)lineNumber);
        UnityPrint("\n    Expected: ");
        UnityPrintFloat(expected);
        UnityPrint(" +/- ");
        UnityPrintFloat(delta);
        UnityPrint("\n    Actual:   ");
        UnityPrintFloat(actual);
        if (msg != NULL) {
            UnityPrint("\n    Message:  ");
            UnityPrint(msg);
        }
        UnityPrint("\n");
    }
}

void UnityAssertDoubleSpecial(const UNITY_DOUBLE actual,
                              const char* msg,
                              const unsigned int lineNumber,
                              const int style)
{
    /* Placeholder for NaN, Inf checks */
    (void)actual;
    (void)msg;
    (void)lineNumber;
    (void)style;
}
#endif
