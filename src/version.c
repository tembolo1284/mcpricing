/*
 * Version information
 *
 * Provides runtime version checking to ensure the header matches the library.
 * Critical for detecting ABI mismatches when the library is updated.
 */

#include "mcoptions.h"

/*
 * Return the compiled version as a packed integer.
 *
 * Format: (major << 16) | (minor << 8) | patch
 *
 * Example: version 2.1.3 = 0x020103
 */
uint32_t mco_version(void)
{
    return MCO_VERSION;
}

/*
 * Return the version as a human-readable string.
 *
 * Example: "mcoptions 2.0.0"
 */
const char *mco_version_string(void)
{
    /* Build the string manually to avoid preprocessor issues */
    static char version_buf[32] = {0};
    
    if (version_buf[0] == '\0') {
        /* First call - build the string */
        char *p = version_buf;
        const char *prefix = "mcoptions ";
        
        /* Copy prefix */
        while (*prefix) {
            *p++ = *prefix++;
        }
        
        /* Major version */
        int major = MCO_VERSION_MAJOR;
        if (major >= 10) *p++ = (char)('0' + (major / 10));
        *p++ = (char)('0' + (major % 10));
        *p++ = '.';
        
        /* Minor version */
        int minor = MCO_VERSION_MINOR;
        if (minor >= 10) *p++ = (char)('0' + (minor / 10));
        *p++ = (char)('0' + (minor % 10));
        *p++ = '.';
        
        /* Patch version */
        int patch = MCO_VERSION_PATCH;
        if (patch >= 10) *p++ = (char)('0' + (patch / 10));
        *p++ = (char)('0' + (patch % 10));
        
        *p = '\0';
    }
    
    return version_buf;
}

/*
 * Check if the compiled library is compatible with the header.
 *
 * Compatibility rules:
 *   - Major version must match exactly
 *   - Minor/patch can differ (minor bumps add features, patches fix bugs)
 *
 * Usage:
 *   if (!mco_is_compatible()) {
 *       fprintf(stderr, "Library version mismatch!\n");
 *       exit(1);
 *   }
 *
 * Returns:
 *   1 if compatible, 0 if not
 */
int mco_is_compatible(void)
{
    uint32_t lib_major = mco_version() >> 16;
    uint32_t hdr_major = MCO_VERSION_MAJOR;
    
    return lib_major == hdr_major;
}
