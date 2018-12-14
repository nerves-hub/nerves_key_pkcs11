/*
 * Copyright (c) 2014-2016 Yubico AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "pkcs11.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "atecc508a.h"

#define CRYTOKI_VERSION { CRYPTOKI_VERSION_MAJOR, CRYPTOKI_VERSION_MINOR * 10 + CRYPTOKI_VERSION_REVISION }
#define NKCS11_VERSION_MAJOR 0
#define NKCS11_VERSION_MINOR 1
#define NKCS11_VERSION_PATCH 0

#define PROGNAME "nerves_key_pkcs11"

#define DEBUG
#ifdef DEBUG
#define ENTER() do { \
        fprintf(stderr, "%s: Entered %s().\r\n", PROGNAME, __func__); \
    } while (0)

#define INFO(fmt, ...) \
        do { fprintf(stderr, "%s: " fmt "\r\n", PROGNAME, ##__VA_ARGS__); } while (0)
#else
#define ENTER()
#define INFO(fmt, ...)
#endif

// Always warn on unimplemented functions. Maybe someone will help make this more complete.
#define UNIMPLEMENTED() do { \
        fprintf(stderr, "%s: %s is unimplemented.\r\n", PROGNAME, __func__); \
    } while (0)

static CK_INFO library_info = {
    .cryptokiVersion = CRYTOKI_VERSION,
    .manufacturerID = "NervesKey",
    .flags = 0,
    .libraryDescription = "PKCS#11 PIV Library (SP-800-73)",
    .libraryVersion = {NKCS11_VERSION_MAJOR, (NKCS11_VERSION_MINOR * 10) + NKCS11_VERSION_PATCH}
};

static CK_SLOT_INFO slot_0_info = {
    .slotDescription = "NervesKey Slot 0",
    .manufacturerID = "NervesKey",
    .flags = CKF_TOKEN_PRESENT | CKF_HW_SLOT,
    .hardwareVersion = {0, 10},
    .firmwareVersion = {0, 10}
};

static CK_TOKEN_INFO slot_0_token_info = {
    .label = "Slot0",
    .manufacturerID = "NervesKey",
    .model = "NervesKey",
    .serialNumber = "FIXME",
    .flags = CKF_WRITE_PROTECTED | CKF_TOKEN_INITIALIZED,
    .ulMaxSessionCount = 1,
    .ulSessionCount = 0,
    .ulMaxRwSessionCount = 0,
    .ulRwSessionCount = 0,
    .ulMaxPinLen = 0,
    .ulMinPinLen = 0,
    .ulTotalPublicMemory = 0, // ??
    .ulFreePublicMemory = 0,
    .ulTotalPrivateMemory = 100,
    .ulFreePrivateMemory = 0,
    .hardwareVersion = {0, 10},
    .firmwareVersion = {0, 10},
    .utcTime = ""
};

static CK_FUNCTION_LIST function_list;

struct nerves_key_session {
    CK_ULONG open_count;
    CK_ULONG find_index;

    int fd;
};

static struct nerves_key_session session;

#define UNUSED(v) (void) v

// See https://www.cryptsoft.com/pkcs11doc/

// https://www.cryptsoft.com/pkcs11doc/v220/pkcs11__all_8h.html

/* General Purpose */

CK_DEFINE_FUNCTION(CK_RV, C_Initialize)(
    CK_VOID_PTR pInitArgs
)
{
    ENTER();
    UNUSED(pInitArgs);

    memset(&session, 0, sizeof(session));
    session.fd = -1;

    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_Finalize)(
    CK_VOID_PTR pReserved
)
{
    ENTER();
    UNUSED(pReserved);
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetInfo)(
    CK_INFO_PTR pInfo
)
{
    ENTER();
    *pInfo = library_info;
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetFunctionList)(
    CK_FUNCTION_LIST_PTR_PTR ppFunctionList
)
{
    ENTER();

    if(ppFunctionList == NULL_PTR) {
        INFO("GetFunctionList called with ppFunctionList = NULL");
        return CKR_ARGUMENTS_BAD;
    }
    *ppFunctionList = &function_list;
    return CKR_OK;
}

/* Slot and token management */

CK_DEFINE_FUNCTION(CK_RV, C_GetSlotList)(
    CK_BBOOL tokenPresent,
    CK_SLOT_ID_PTR pSlotList,
    CK_ULONG_PTR pulCount
)
{
    UNUSED(tokenPresent);

    ENTER();

    if (pSlotList == NULL_PTR) {
        *pulCount = 1;
    } else {
        if (*pulCount < 1)
            return CKR_BUFFER_TOO_SMALL;
        *pulCount = 1;
        *pSlotList = 0; // Slot 0 is the only slot
    }
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetSlotInfo)(
    CK_SLOT_ID slotID,
    CK_SLOT_INFO_PTR pInfo
)
{
    ENTER();
    if (slotID != 0)
        return CKR_SLOT_ID_INVALID;

    *pInfo = slot_0_info;
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetTokenInfo)(
    CK_SLOT_ID slotID,
    CK_TOKEN_INFO_PTR pInfo
)
{
    UNUSED(slotID);
    UNUSED(pInfo);
    ENTER();
    if (slotID != 0)
        return CKR_SLOT_ID_INVALID;
    *pInfo = slot_0_token_info;
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_WaitForSlotEvent)(
    CK_FLAGS flags,
    CK_SLOT_ID_PTR pSlot,
    CK_VOID_PTR pReserved
)
{
    UNUSED(flags);
    UNUSED(pSlot);
    UNUSED(pReserved);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetMechanismList)(
    CK_SLOT_ID slotID,
    CK_MECHANISM_TYPE_PTR pMechanismList,
    CK_ULONG_PTR pulCount
)
{
    UNUSED(slotID);
    UNUSED(pMechanismList);
    UNUSED(pulCount);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetMechanismInfo)(
    CK_SLOT_ID slotID,
    CK_MECHANISM_TYPE type,
    CK_MECHANISM_INFO_PTR pInfo
)
{
    UNUSED(slotID);
    UNUSED(type);
    UNUSED(pInfo);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_InitToken)(
    CK_SLOT_ID slotID,
    CK_UTF8CHAR_PTR pPin,
    CK_ULONG ulPinLen,
    CK_UTF8CHAR_PTR pLabel
)
{
    UNUSED(slotID);
    UNUSED(pPin);
    UNUSED(ulPinLen);
    UNUSED(pLabel);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_InitPIN)(
    CK_SESSION_HANDLE hSession,
    CK_UTF8CHAR_PTR pPin,
    CK_ULONG ulPinLen
)
{
    UNUSED(hSession);
    UNUSED(pPin);
    UNUSED(ulPinLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_SetPIN)(
    CK_SESSION_HANDLE hSession,
    CK_UTF8CHAR_PTR pOldPin,
    CK_ULONG ulOldLen,
    CK_UTF8CHAR_PTR pNewPin,
    CK_ULONG ulNewLen
)
{
    UNUSED(hSession);
    UNUSED(pOldPin);
    UNUSED(ulOldLen);
    UNUSED(pNewPin);
    UNUSED(ulNewLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_OpenSession)(
    CK_SLOT_ID slotID,
    CK_FLAGS flags,
    CK_VOID_PTR pApplication,
    CK_NOTIFY Notify,
    CK_SESSION_HANDLE_PTR phSession
)
{
    ENTER();
    if (slotID != 0)
        return CKR_SLOT_ID_INVALID;
    if (phSession == NULL_PTR)
        return CKR_ARGUMENTS_BAD;
    if ((flags & CKF_SERIAL_SESSION) == 0)
        return CKR_SESSION_PARALLEL_NOT_SUPPORTED;

    if ((flags & CKF_RW_SESSION))
        return CKR_TOKEN_WRITE_PROTECTED;

    // Unused
    UNUSED(pApplication);
    UNUSED(Notify);

    phSession = 0;

    if (session.open_count == 0) {
        session.fd = atecc508a_open("/dev/i2c-1");
        if (session.fd < 0) {
            INFO("Error opening I2C bus: %s", "/dev/i2c-1");
            return CKR_DEVICE_ERROR;
        }
    }
    session.open_count++;
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_CloseSession)(
    CK_SESSION_HANDLE hSession
)
{
    ENTER();

    if (hSession != 0 || session.open_count == 0)
        return CKR_SLOT_ID_INVALID;

    session.open_count--;
    if (session.open_count == 0) {
        atecc508a_close(session.fd);
        session.fd = -1;
    }
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_CloseAllSessions)(
    CK_SLOT_ID slotID
)
{
    ENTER();
    if (slotID != 0)
        return CKR_SLOT_ID_INVALID;

    if (session.open_count > 0) {
        atecc508a_close(session.fd);
        session.open_count = 0;
        session.fd = -1;
    }
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetSessionInfo)(
    CK_SESSION_HANDLE hSession,
    CK_SESSION_INFO_PTR pInfo
)
{
    UNUSED(hSession);
    UNUSED(pInfo);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetOperationState)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pOperationState,
    CK_ULONG_PTR pulOperationStateLen
)
{
    UNUSED(hSession);
    UNUSED(pOperationState);
    UNUSED(pulOperationStateLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_SetOperationState)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pOperationState,
    CK_ULONG ulOperationStateLen,
    CK_OBJECT_HANDLE hEncryptionKey,
    CK_OBJECT_HANDLE hAuthenticationKey
)
{
    UNUSED(hSession);
    UNUSED(pOperationState);
    UNUSED(ulOperationStateLen);
    UNUSED(hEncryptionKey);
    UNUSED(hAuthenticationKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_Login)(
    CK_SESSION_HANDLE hSession,
    CK_USER_TYPE userType,
    CK_UTF8CHAR_PTR pPin,
    CK_ULONG ulPinLen
)
{
    UNUSED(hSession);
    UNUSED(userType);
    UNUSED(pPin);
    UNUSED(ulPinLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_Logout)(
    CK_SESSION_HANDLE hSession
)
{
    UNUSED(hSession);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_CreateObject)(
    CK_SESSION_HANDLE hSession,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulCount,
    CK_OBJECT_HANDLE_PTR phObject
)
{
    UNUSED(hSession);
    UNUSED(pTemplate);
    UNUSED(ulCount);
    UNUSED(phObject);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_CopyObject)(
    CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulCount,
    CK_OBJECT_HANDLE_PTR phNewObject
)
{
    UNUSED(hSession);
    UNUSED(hObject);
    UNUSED(pTemplate);
    UNUSED(ulCount);
    UNUSED(phNewObject);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DestroyObject)(
    CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject
)
{
    UNUSED(hSession);
    UNUSED(hObject);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetObjectSize)(
    CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject,
    CK_ULONG_PTR pulSize
)
{
    UNUSED(hSession);
    UNUSED(hObject);
    UNUSED(pulSize);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetAttributeValue)(
    CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulCount
)
{
    UNUSED(hSession);
    UNUSED(hObject);
    ENTER();
    if (hSession != 0 || session.open_count == 0)
        return CKR_SESSION_HANDLE_INVALID;

    if (pTemplate == NULL_PTR || ulCount == 0)
        return CKR_ARGUMENTS_BAD;

    CK_RV rv_final = CKR_OK;
    for (CK_ULONG i = 0; i < ulCount; i++) {
        CK_RV rv;

        switch (pTemplate[i].type) {
        case CKA_KEY_TYPE:
            // Type of key.
            if (pTemplate[i].pValue == NULL_PTR) {
                pTemplate[i].ulValueLen = sizeof(CK_ULONG);
                rv = CKR_OK;
            } else if (pTemplate[i].ulValueLen >= sizeof(CK_ULONG)) {
                pTemplate[i].ulValueLen = sizeof(CK_ULONG);
                *((CK_ULONG *) pTemplate[i].pValue) = CKK_ECDSA;
                rv = CKR_OK;
            } else {
                pTemplate[i].ulValueLen = (CK_ULONG) -1;
                rv = CKR_BUFFER_TOO_SMALL;
            }
            break;

        case CKA_LABEL:
            // Description of the object (default empty).
            if (pTemplate[i].pValue == NULL_PTR) {
                pTemplate[i].ulValueLen = 3;
                rv = CKR_OK;
            } else if (pTemplate[i].ulValueLen >= 3) {
                pTemplate[i].ulValueLen = 3;
                strcpy((char *) pTemplate[i].pValue, "0");
                rv = CKR_OK;
            } else {
                pTemplate[i].ulValueLen = (CK_ULONG) -1;
                rv = CKR_BUFFER_TOO_SMALL;
            }
            break;

        case CKA_ID:
            // Key identifier for public/private key pair (default empty).
            if (pTemplate[i].pValue == NULL_PTR) {
                pTemplate[i].ulValueLen = 1;
                rv = CKR_OK;
            } else if (pTemplate[i].ulValueLen >= 1) {
                pTemplate[i].ulValueLen = 1;
                *((CK_BYTE *) pTemplate[i].pValue) = '0';
                rv = CKR_OK;
            } else {
                pTemplate[i].ulValueLen = (CK_ULONG) -1;
                rv = CKR_BUFFER_TOO_SMALL;
            }
            break;

        case CKA_EC_PARAMS:
            // DER-encoding of an ANSI X9.62 Parameters value.
        {
            static const CK_BYTE prime256v1[] = "\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07";
            if (pTemplate[i].pValue == NULL_PTR) {
                pTemplate[i].ulValueLen = sizeof(prime256v1);
                rv = CKR_OK;
            } else if (pTemplate[i].ulValueLen >= sizeof(prime256v1)) {
                pTemplate[i].ulValueLen = sizeof(prime256v1);
                memcpy(pTemplate[i].pValue, prime256v1, sizeof(prime256v1));
                rv = CKR_OK;
            } else {
                pTemplate[i].ulValueLen = (CK_ULONG) -1;
                rv = CKR_BUFFER_TOO_SMALL;
            }
            break;
        }
        case CKA_EC_POINT:
            // DER-encoding of ANSI X9.62 ECPoint value ''Q''.
        {
            CK_BYTE publickey[65] = {0};

            if (pTemplate[i].pValue == NULL_PTR) {
                pTemplate[i].ulValueLen = sizeof(publickey);
                rv = CKR_OK;
            } else if (pTemplate[i].ulValueLen >= sizeof(publickey)) {
                pTemplate[i].ulValueLen = sizeof(publickey);

                // DER-encoding of the key starts with 0x04
                publickey[0] = 0x04;
                if (atecc508a_derive_public_key(session.fd, 0, &publickey[1]) < 0) {
                    INFO("Error getting public key!");
                    rv = CKR_DEVICE_ERROR;
                } else {
                    rv = CKR_OK;
                }
            } else {
                pTemplate[i].ulValueLen = (CK_ULONG) -1;
                rv = CKR_BUFFER_TOO_SMALL;
            }
            break;
        }

        case CKA_ALWAYS_AUTHENTICATE:
            // 	If CK_TRUE, the user has to supply the PIN for each use (sign or decrypt) with the key.
            if (pTemplate[i].pValue == NULL_PTR) {
                pTemplate[i].ulValueLen = sizeof(CK_BBOOL);
                rv = CKR_OK;
            } else if (pTemplate[i].ulValueLen >=  sizeof(CK_BBOOL)) {
                pTemplate[i].ulValueLen =  sizeof(CK_BBOOL);
                *((CK_BBOOL *) pTemplate[i].pValue) = CK_FALSE;
                rv = CKR_OK;
            } else {
                pTemplate[i].ulValueLen = (CK_ULONG) -1;
                rv = CKR_BUFFER_TOO_SMALL;
            }
            break;

        case CKA_SIGN:
            // 	CK_TRUE if key supports signatures where the signature is an appendix to the data.
            if (pTemplate[i].pValue == NULL_PTR) {
                pTemplate[i].ulValueLen = sizeof(CK_BBOOL);
                rv = CKR_OK;
            } else if (pTemplate[i].ulValueLen >=  sizeof(CK_BBOOL)) {
                pTemplate[i].ulValueLen =  sizeof(CK_BBOOL);
                *((CK_BBOOL *) pTemplate[i].pValue) = CK_TRUE;
                rv = CKR_OK;
            } else {
                pTemplate[i].ulValueLen = (CK_ULONG) -1;
                rv = CKR_BUFFER_TOO_SMALL;
            }
            break;

        default:
            pTemplate[i].ulValueLen = (CK_ULONG) -1;
            rv = CKR_ATTRIBUTE_TYPE_INVALID;
            break;
        }
        if (rv != CKR_OK) {
            INFO("Unable to get attribute 0x%lx of object %lu", pTemplate[i].type, hObject);
            rv_final = rv;
        }
    }
    return rv_final;
}

CK_DEFINE_FUNCTION(CK_RV, C_SetAttributeValue)(
    CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulCount
)
{
    UNUSED(hSession);
    UNUSED(hObject);
    ENTER();
    if (hSession != 0 || session.open_count == 0)
        return CKR_SESSION_HANDLE_INVALID;

    if (pTemplate == NULL_PTR || ulCount == 0)
        return CKR_ARGUMENTS_BAD;

    CK_RV rv_final = CKR_OK;
    for (CK_ULONG i = 0; i < ulCount; i++) {
        CK_RV rv = CKR_ATTRIBUTE_TYPE_INVALID;

        // TODO: this function has some complex cases for return vlaue. Make sure to check them.
        if (rv != CKR_OK) {
            INFO("Unable to set attribute 0x%lx of object %lu", pTemplate[i].type, hObject);
            rv_final = rv;
        }
    }
    return rv_final;
}

CK_DEFINE_FUNCTION(CK_RV, C_FindObjectsInit)(
    CK_SESSION_HANDLE hSession,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulCount
)
{
    UNUSED(hSession);
    UNUSED(pTemplate);
    UNUSED(ulCount);
    ENTER();

    if (hSession != 0 || session.open_count == 0)
        return CKR_SESSION_HANDLE_INVALID;

    session.find_index = 0;
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_FindObjects)(
    CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE_PTR phObject,
    CK_ULONG ulMaxObjectCount,
    CK_ULONG_PTR pulObjectCount
)
{
    ENTER();
    if (hSession != 0 || session.open_count == 0)
        return CKR_SESSION_HANDLE_INVALID;

    if (ulMaxObjectCount > 0 && session.find_index == 0) {
        *phObject = 0;
        *pulObjectCount = 1;
        session.find_index++;
    } else {
        *pulObjectCount = 0;
    }
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_FindObjectsFinal)(
    CK_SESSION_HANDLE hSession
)
{
    UNUSED(hSession);
    ENTER();
    if (hSession != 0 || session.open_count == 0)
        return CKR_SESSION_HANDLE_INVALID;
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_EncryptInit)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_OBJECT_HANDLE hKey
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNUSED(hKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_Encrypt)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,
    CK_ULONG ulDataLen,
    CK_BYTE_PTR pEncryptedData,
    CK_ULONG_PTR pulEncryptedDataLen
)
{
    UNUSED(hSession);
    UNUSED(pData);
    UNUSED(ulDataLen);
    UNUSED(pEncryptedData);
    UNUSED(pulEncryptedDataLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_EncryptUpdate)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart,
    CK_ULONG ulPartLen,
    CK_BYTE_PTR pEncryptedPart,
    CK_ULONG_PTR pulEncryptedPartLen
)
{
    UNUSED(hSession);
    UNUSED(pPart);
    UNUSED(ulPartLen);
    UNUSED(pEncryptedPart);
    UNUSED(pulEncryptedPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_EncryptFinal)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastEncryptedPart,
    CK_ULONG_PTR pulLastEncryptedPartLen
)
{
    UNUSED(hSession);
    UNUSED(pLastEncryptedPart);
    UNUSED(pulLastEncryptedPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DecryptInit)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_OBJECT_HANDLE hKey
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNUSED(hKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_Decrypt)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedData,
    CK_ULONG ulEncryptedDataLen,
    CK_BYTE_PTR pData,
    CK_ULONG_PTR pulDataLen
)
{
    UNUSED(hSession);
    UNUSED(pEncryptedData);
    UNUSED(ulEncryptedDataLen);
    UNUSED(pData);
    UNUSED(pulDataLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DecryptUpdate)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedPart,
    CK_ULONG ulEncryptedPartLen,
    CK_BYTE_PTR pPart,
    CK_ULONG_PTR pulPartLen
)
{
    UNUSED(hSession);
    UNUSED(pEncryptedPart);
    UNUSED(ulEncryptedPartLen);
    UNUSED(pPart);
    UNUSED(pulPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DecryptFinal)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastPart,
    CK_ULONG_PTR pulLastPartLen
)
{
    UNUSED(hSession);
    UNUSED(pLastPart);
    UNUSED(pulLastPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DigestInit)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_Digest)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,
    CK_ULONG ulDataLen,
    CK_BYTE_PTR pDigest,
    CK_ULONG_PTR pulDigestLen
)
{
    UNUSED(hSession);
    UNUSED(pData);
    UNUSED(ulDataLen);
    UNUSED(pDigest);
    UNUSED(pulDigestLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DigestUpdate)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart,
    CK_ULONG ulPartLen
)
{
    UNUSED(hSession);
    UNUSED(pPart);
    UNUSED(ulPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DigestKey)(
    CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hKey
)
{
    UNUSED(hSession);
    UNUSED(hKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DigestFinal)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pDigest,
    CK_ULONG_PTR pulDigestLen
)
{
    UNUSED(hSession);
    UNUSED(pDigest);
    UNUSED(pulDigestLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_SignInit)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_OBJECT_HANDLE hKey
)
{
    UNUSED(hSession);
    UNUSED(hKey);
    ENTER();
    if (hSession != 0 || session.open_count == 0)
        return CKR_SESSION_HANDLE_INVALID;
    if (pMechanism == NULL_PTR)
        return CKR_ARGUMENTS_BAD;

    CK_RV rv;
    switch (pMechanism->mechanism) {
    case CKM_ECDSA:
        rv = CKR_OK;
        break;

    default:
        rv = CKR_MECHANISM_INVALID;
        break;
    }
    return rv;
}

CK_DEFINE_FUNCTION(CK_RV, C_Sign)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,
    CK_ULONG ulDataLen,
    CK_BYTE_PTR pSignature,
    CK_ULONG_PTR pulSignatureLen
)
{
    UNUSED(hSession);
    ENTER();
    if (hSession != 0 || session.open_count == 0)
        return CKR_SESSION_HANDLE_INVALID;
    if (pulSignatureLen == NULL_PTR)
        return CKR_ARGUMENTS_BAD;

    if (pSignature == NULL_PTR) {
        *pulSignatureLen = 64;
        return CKR_OK;
    } else if (*pulSignatureLen < 64) {
        *pulSignatureLen = 64;
        return CKR_BUFFER_TOO_SMALL;
    }

    if (pData == NULL_PTR)
        return CKR_ARGUMENTS_BAD;

    if (ulDataLen != 32) {
        INFO("C_Sign called with unsupported data length: %lu", ulDataLen);
        return CKR_ARGUMENTS_BAD;
    }

    if (atecc508a_sign(session.fd, 0, pData, pSignature) < 0) {
        INFO("Error signing data!");
        return CKR_DEVICE_ERROR;
    }
    *pulSignatureLen = 64;
    return CKR_OK;
}

CK_DEFINE_FUNCTION(CK_RV, C_SignUpdate)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart,
    CK_ULONG ulPartLen
)
{
    UNUSED(hSession);
    UNUSED(pPart);
    UNUSED(ulPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_SignFinal)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pSignature,
    CK_ULONG_PTR pulSignatureLen
)
{
    UNUSED(hSession);
    UNUSED(pSignature);
    UNUSED(pulSignatureLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_SignRecoverInit)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_OBJECT_HANDLE hKey
)
{
    UNUSED(hSession);
    UNUSED(hKey);
    UNUSED(pMechanism);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_SignRecover)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,
    CK_ULONG ulDataLen,
    CK_BYTE_PTR pSignature,
    CK_ULONG_PTR pulSignatureLen
)
{
    UNUSED(hSession);
    UNUSED(pData);
    UNUSED(ulDataLen);
    UNUSED(pSignature);
    UNUSED(pulSignatureLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_VerifyInit)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_OBJECT_HANDLE hKey
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNUSED(hKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_Verify)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,
    CK_ULONG ulDataLen,
    CK_BYTE_PTR pSignature,
    CK_ULONG ulSignatureLen
)
{
    UNUSED(hSession);
    UNUSED(pData);
    UNUSED(ulDataLen);
    UNUSED(pSignature);
    UNUSED(ulSignatureLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_VerifyUpdate)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart,
    CK_ULONG ulPartLen
)
{
    UNUSED(hSession);
    UNUSED(pPart);
    UNUSED(ulPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_VerifyFinal)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pSignature,
    CK_ULONG ulSignatureLen
)
{
    UNUSED(hSession);
    UNUSED(pSignature);
    UNUSED(ulSignatureLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_VerifyRecoverInit)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_OBJECT_HANDLE hKey
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNUSED(hKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_VerifyRecover)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pSignature,
    CK_ULONG ulSignatureLen,
    CK_BYTE_PTR pData,
    CK_ULONG_PTR pulDataLen
)
{
    UNUSED(hSession);
    UNUSED(pSignature);
    UNUSED(ulSignatureLen);
    UNUSED(pData);
    UNUSED(pulDataLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DigestEncryptUpdate)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart,
    CK_ULONG ulPartLen,
    CK_BYTE_PTR pEncryptedPart,
    CK_ULONG_PTR pulEncryptedPartLen
)
{
    UNUSED(hSession);
    UNUSED(pPart);
    UNUSED(ulPartLen);
    UNUSED(pEncryptedPart);
    UNUSED(pulEncryptedPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DecryptDigestUpdate)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedPart,
    CK_ULONG ulEncryptedPartLen,
    CK_BYTE_PTR pPart,
    CK_ULONG_PTR pulPartLen
)
{
    UNUSED(pPart);
    UNUSED(pEncryptedPart);
    UNUSED(ulEncryptedPartLen);
    UNUSED(pulPartLen);
    UNUSED(hSession);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_SignEncryptUpdate)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart,
    CK_ULONG ulPartLen,
    CK_BYTE_PTR pEncryptedPart,
    CK_ULONG_PTR pulEncryptedPartLen
)
{
    UNUSED(hSession);
    UNUSED(pPart);
    UNUSED(ulPartLen);
    UNUSED(pEncryptedPart);
    UNUSED(pulEncryptedPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DecryptVerifyUpdate)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedPart,
    CK_ULONG ulEncryptedPartLen,
    CK_BYTE_PTR pPart,
    CK_ULONG_PTR pulPartLen
)
{
    UNUSED(hSession);
    UNUSED(pPart);
    UNUSED(pEncryptedPart);
    UNUSED(ulEncryptedPartLen);
    UNUSED(pulPartLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_GenerateKey)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulCount,
    CK_OBJECT_HANDLE_PTR phKey
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNUSED(pTemplate);
    UNUSED(ulCount);
    UNUSED(phKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_GenerateKeyPair)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_ATTRIBUTE_PTR pPublicKeyTemplate,
    CK_ULONG ulPublicKeyAttributeCount,
    CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
    CK_ULONG ulPrivateKeyAttributeCount,
    CK_OBJECT_HANDLE_PTR phPublicKey,
    CK_OBJECT_HANDLE_PTR phPrivateKey
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNUSED(pPublicKeyTemplate);
    UNUSED(ulPublicKeyAttributeCount);
    UNUSED(pPrivateKeyTemplate);
    UNUSED(ulPrivateKeyAttributeCount);
    UNUSED(phPublicKey);
    UNUSED(phPrivateKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_WrapKey)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_OBJECT_HANDLE hWrappingKey,
    CK_OBJECT_HANDLE hKey,
    CK_BYTE_PTR pWrappedKey,
    CK_ULONG_PTR pulWrappedKeyLen
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNUSED(hWrappingKey);
    UNUSED(hKey);
    UNUSED(pWrappedKey);
    UNUSED(pulWrappedKeyLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_UnwrapKey)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_OBJECT_HANDLE hUnwrappingKey,
    CK_BYTE_PTR pWrappedKey,
    CK_ULONG ulWrappedKeyLen,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulAttributeCount,
    CK_OBJECT_HANDLE_PTR phKey
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNUSED(hUnwrappingKey);
    UNUSED(pWrappedKey);
    UNUSED(ulWrappedKeyLen);
    UNUSED(pTemplate);
    UNUSED(ulAttributeCount);
    UNUSED(phKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_DeriveKey)(
    CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,
    CK_OBJECT_HANDLE hBaseKey,
    CK_ATTRIBUTE_PTR pTemplate,
    CK_ULONG ulAttributeCount,
    CK_OBJECT_HANDLE_PTR phKey
)
{
    UNUSED(hSession);
    UNUSED(pMechanism);
    UNUSED(hBaseKey);
    UNUSED(pTemplate);
    UNUSED(ulAttributeCount);
    UNUSED(phKey);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

/* Random number generation functions */

CK_DEFINE_FUNCTION(CK_RV, C_SeedRandom)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pSeed,
    CK_ULONG ulSeedLen
)
{
    UNUSED(hSession);
    UNUSED(pSeed);
    UNUSED(ulSeedLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_GenerateRandom)(
    CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pRandomData,
    CK_ULONG ulRandomLen
)
{
    UNUSED(hSession);
    UNUSED(pRandomData);
    UNUSED(ulRandomLen);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_GetFunctionStatus)(
    CK_SESSION_HANDLE hSession
)
{
    UNUSED(hSession);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

CK_DEFINE_FUNCTION(CK_RV, C_CancelFunction)(
    CK_SESSION_HANDLE hSession
)
{
    UNUSED(hSession);
    UNIMPLEMENTED();
    return CKR_FUNCTION_FAILED;
}

static CK_FUNCTION_LIST function_list = {
    CRYTOKI_VERSION,
    C_Initialize,
    C_Finalize,
    C_GetInfo,
    C_GetFunctionList,
    C_GetSlotList,
    C_GetSlotInfo,
    C_GetTokenInfo,
    C_GetMechanismList,
    C_GetMechanismInfo,
    C_InitToken,
    C_InitPIN,
    C_SetPIN,
    C_OpenSession,
    C_CloseSession,
    C_CloseAllSessions,
    C_GetSessionInfo,
    C_GetOperationState,
    C_SetOperationState,
    C_Login,
    C_Logout,
    C_CreateObject,
    C_CopyObject,
    C_DestroyObject,
    C_GetObjectSize,
    C_GetAttributeValue,
    C_SetAttributeValue,
    C_FindObjectsInit,
    C_FindObjects,
    C_FindObjectsFinal,
    C_EncryptInit,
    C_Encrypt,
    C_EncryptUpdate,
    C_EncryptFinal,
    C_DecryptInit,
    C_Decrypt,
    C_DecryptUpdate,
    C_DecryptFinal,
    C_DigestInit,
    C_Digest,
    C_DigestUpdate,
    C_DigestKey,
    C_DigestFinal,
    C_SignInit,
    C_Sign,
    C_SignUpdate,
    C_SignFinal,
    C_SignRecoverInit,
    C_SignRecover,
    C_VerifyInit,
    C_Verify,
    C_VerifyUpdate,
    C_VerifyFinal,
    C_VerifyRecoverInit,
    C_VerifyRecover,
    C_DigestEncryptUpdate,
    C_DecryptDigestUpdate,
    C_SignEncryptUpdate,
    C_DecryptVerifyUpdate,
    C_GenerateKey,
    C_GenerateKeyPair,
    C_WrapKey,
    C_UnwrapKey,
    C_DeriveKey,
    C_SeedRandom,
    C_GenerateRandom,
    C_GetFunctionStatus,
    C_CancelFunction,
    C_WaitForSlotEvent,
};
