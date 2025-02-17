// Unit Include
#include "lr-install-interface.h"

// Windows Includes (Specifically for changing file permissions)
#include "Aclapi.h"

namespace Lr
{
//===============================================================================================================
// IInstall
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Private:
void IInstall::ensureModifiable(const QString& filePath)
{
    Q_ASSERT(!filePath.isEmpty());
    PACL pCurrentDacl = nullptr, pNewDACL = nullptr;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = nullptr;
    PSID pOwnerId = nullptr;

    // Ensure cleanup
    QScopeGuard cleanup([&]{
        LocalFree(pSecurityDescriptor);
        LocalFree(pNewDACL);
    });

    /* NOTE: We do two things here that are technically risky, but should be ok:
     *
     * 1) We get a pointer to the underlying data of the QString and cast it to const wchar_t*.
     *    on other platforms the size of wchar_t can vary, but on Windows it's clear that it's
     *    2-bytes, as it even caused a defect in the C++ standard for being so.
     * 2) For some reason SetNamedSecurityInfo() takes the path string as non-const, which I'm
     *    almost certain is an oversight, as the docs make it clear its just an input to be
     *    read; therefore, we cast away the constness.
     */
    LPCWSTR cPath = reinterpret_cast<const wchar_t*>(filePath.data());
    DWORD status;

    if(status = GetNamedSecurityInfo(
        cPath,
        SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
        &pOwnerId,
        NULL,
        &pCurrentDacl,
        NULL,
        &pSecurityDescriptor
    ); status != ERROR_SUCCESS)
    {
        qWarning("Failed to query install file security info: 0x%X", status);
        return;
    }

    EXPLICIT_ACCESS access{
        .grfAccessPermissions = GENERIC_ALL,
        .grfAccessMode = SET_ACCESS,
        .grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
        .Trustee = {
            .pMultipleTrustee = NULL,
            .MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE,
            .TrusteeForm = TRUSTEE_IS_SID,
            .TrusteeType = TRUSTEE_IS_UNKNOWN,
            .ptstrName = (LPTSTR) pOwnerId
        }
    };

    if(status = SetEntriesInAcl(1, &access, pCurrentDacl, &pNewDACL); status != ERROR_SUCCESS)
    {
        qWarning("Failed to update install file ACL: 0x%X", status);
        return;
    }

    if(status = SetNamedSecurityInfo(
        const_cast<wchar_t*>(cPath),
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        NULL,
        NULL,
        pNewDACL,
        NULL
        ); status != ERROR_SUCCESS)
    {
        qWarning("Failed to save install file ACLs: 0x%X", status);
        return;
    }
}

}
