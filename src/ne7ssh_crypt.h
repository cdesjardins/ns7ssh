/***************************************************************************
 *   Copyright (C) 2005-2007 by NetSieben Technologies INC                 *
 *   Author: Andrew Useckas                                                *
 *   Email: andrew@netsieben.com                                           *
 *                                                                         *
 *   Windows Port and bugfixes: Keef Aragon <keef@netsieben.com>           *
 *                                                                         *
 *   This program may be distributed under the terms of the Q Public       *
 *   License as defined by Trolltech AS of Norway and appearing in the     *
 *   file LICENSE.QPL included in the packaging of this file.              *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 ***************************************************************************/

#ifndef CRYPT_H
#define CRYPT_H

#include "ne7ssh_string.h"

#include <botan/dh.h>
#include <botan/dsa.h>
#include <botan/rsa.h>

// #if defined(BOTAN_EXT_COMPRESSOR_ZLIB)
//   #include <botan/zlib.h>
// #else
//   #error "Zlib support is not compiled into Botan"
// #endif

#include <botan/hmac.h>
#include <memory>

class ne7ssh_session;

/**
@author Andrew Useckas
*/
class ne7ssh_crypt
{
private:
    std::shared_ptr<ne7ssh_session> _session;

    enum kexMethods { DH_GROUP1_SHA1, DH_GROUP14_SHA1 };
    uint32 _kexMethod;

    enum hostkeyMethods { SSH_DSS, SSH_RSA };
    uint32 _hostkeyMethod;

    enum cryptoMethods { TDES_CBC, AES128_CBC, AES192_CBC, AES256_CBC, BLOWFISH_CBC, CAST128_CBC, TWOFISH_CBC };
    uint32 _c2sCryptoMethod;
    uint32 _s2cCryptoMethod;

    enum macMethods { HMAC_SHA1, HMAC_MD5, HMAC_NONE };
    uint32 _c2sMacMethod;
    uint32 _s2cMacMethod;

    enum cmprsMethods { NONE, ZLIB };
    uint32 _c2sCmprsMethod;
    uint32 _s2cCmprsMethod;

    bool _inited;
    Botan::SecureVector<Botan::byte> _H;
    Botan::SecureVector<Botan::byte> _K;

    std::unique_ptr<Botan::Pipe> _encrypt;
    std::unique_ptr<Botan::Pipe> _decrypt;
    std::unique_ptr<Botan::Pipe> _compress;
    std::unique_ptr<Botan::Pipe> _decompress;
    std::unique_ptr<Botan::HMAC> _hmacOut;
    std::unique_ptr<Botan::HMAC> _hmacIn;

    std::unique_ptr<Botan::DH_PrivateKey> _privKexKey;

    uint32 _encryptBlock;
    uint32 _decryptBlock;

    /**
     * Generates a new Public Key, based on Diffie Helman Group1, SHA1 standard.
     * @param publicKey Reference to publick Key. The result will be dumped into this var.
     * @return If generation successful returns true, otherwise false is returned.
     */
    bool getDHGroup1Sha1Public(Botan::BigInt& publicKey);

    /**
     * Generates a new Public Key, based on Diffie Helman Group14, SHA1 standard.
     * @param publicKey Reference to publick Key. The result will be dumped into this var.
     * @return If generation successful returns true, otherwise false is returned.
     */
    bool getDHGroup14Sha1Public(Botan::BigInt &publicKey);

    /**
     * Generates a new DSA public Key from p,q,g,y values extracted from the host key received from the server.
     * @param hostKey Reference to vector containing host key received from a server.
     * @return Returns newly generated DSA public Key. If host key vector is trash, more likely application will be aborted within Botan library.
     */
    std::shared_ptr<Botan::DSA_PublicKey> getDSAKey(Botan::SecureVector<Botan::byte>& hostKey);

    /**
    * Generates a new RSA public Key from n and e values extracted from the host key received from the server.
    * @param hostKey Reference to a vector containing host key.
    * @return Returns newly generated ESA public Key. If the hostkey is trash, more likely application will abort within Botan library.
    */
    std::shared_ptr<Botan::RSA_PublicKey> getRSAKey(Botan::SecureVector<Botan::byte> &hostKey);

    /**
     * Returns a string represenation of negotiated one way hash algorithm. For DH1_GROUP1_SHA1, "SHA-1" will be returned.
     * @return A string containing algorithm name.
     */
    const char* getHashAlgo();

    /**
     * Returns a string represenation of negotiated cipher algorithm.
     * @param crypto Integer represenating a cipher algorithm.
     * @return A string containing algorithm name.
     */
    const char* getCryptAlgo(uint32 crypto);

    /**
     * Returns a string represenation of negotiated HMAC algorithm.
     * @param method Integer represenating HMAC algorithm.
     * @return A string containing algorithm name.
     */
    const char* getHmacAlgo(uint32 method);

    /**
     * Returns key length of the negotiated HMAC algorithm.
     * <p> Used in HMAC key generation. Key length coded in accordance with SSH protocol specs.
     * @param method Integer represenating HMAC algorithm.
     * @return Key length.
     */
    uint32 getMacKeyLen(uint32 method);

    /**
     * Returns digest length of negotiated HMAC algorithm.
     * <p> If HMAC integrity checking is enabled, this value is used in the verification process. Digest length coded in accordance with SSH protocol specs.
     * @param method Integer represenation of HMAC algorithm.
     * @return Key length.
     */
    uint32 getMacDigestLen(uint32 method);

    /**
     * Function used to compute crypto and HMAC keys.
     * <p> Keys are computed using K, H, ID and sessionID values. All these are concated and hashed. If hash is not long enough K, H, and newly generated key is used over and over again, till keys are long enough.
     * @param key Resulting key will be dumped into this var.
     * @param ID Single character ID, as specified in SSH protocol specs.
     * @param nBytes Key length in bytes.
     * @return True if key generation was successful, otherwise false is returned.
     */
    bool compute_key(Botan::SecureVector<Botan::byte>& key, Botan::byte ID, uint32 nBytes);

    size_t max_keylength_of(const std::string& name);

public:

    /**
     * ne7ssh_crypt class constructor.
     * @param _session Pointer to ne7ssh_session class.
     */
    ne7ssh_crypt(std::shared_ptr<ne7ssh_session> session);

    /**
     * ne7ssh_crypt class destructor.
     */
    ~ne7ssh_crypt();

    /**
     * Checks if cryptographic engine has been initialized.
     * <p> The engine is initialized when all crypto and hmac keys are generated and the cryptographic Pipes are created.
     * @return True if cryptographic engine is initialized, otherwise false is returned.
     */
    bool isInited()
    {
        return _inited;
    }

    /**
     * Returns the size of encryption block.
     * @return Size of the block.
     */
    uint32 getEncryptBlock()
    {
        return _encryptBlock;
    }

    /**
     * Returns the size of decryption block
     * @return Size of the block
     */
    uint32 getDecryptBlock()
    {
        return _decryptBlock;
    }

    /**
     * Returns digest length of HMAC algorithm used to verify integrity of transmited data.
     * @return Digest length.
     */
    uint32 getMacOutLen()
    {
        return getMacDigestLen(_c2sMacMethod);
    }

    /**
     * Returns digest length of HMAC algorithm used to verify integrity of data received.
     * @return Digest length.
     */
    uint32 getMacInLen()
    {
        return getMacDigestLen(_s2cMacMethod);
    }

    /**
     * This function is used in negotiations of crypto, signing and HMAC algorithms.
     * @param result Reference to a vector where negotiated algorithm name will be dumped.
     * @param local String containing a list of localy supported algorithms, separated by commas.
     * @param remote Reference to vector containing a list of algorithms supported by remote side, separated by commas.
     * @return True, if common algorithm was found, otherwise false is returned.
     */
    bool agree(Botan::SecureVector<Botan::byte>& result, const char* local, Botan::SecureVector<Botan::byte>& remote);

    /**
     * Parses negotiated key exchange algorithm vector, and registers it's integer representation with the class.
     * @param kexAlgo Reference to a vector containing the negotiated KEX algorithm.
     * @return True if negotiated algorithm is recognized, otherwise false is returned.
     */
    bool negotiatedKex(Botan::SecureVector<Botan::byte>& kexAlgo);

    /**
     * Parses negotiated host key algorithm and registers it's integer representation with the class.
     * @param hostKeyAlgo Reference to a vector containing the negotiated host key algorithm.
     * @return True if negotiated algorithm is recognized, otherwise false is returned.
     */
    bool negotiatedHostkey(Botan::SecureVector<Botan::byte>& hostKeyAlgo);

    /**
     * Parse negotiated client to server cipher algorithm and registers it's integer representation with the class.
     * @param cryptoAlgo Reference to a vector containing the negotiated cipher algorithm.
     * @return True if negotiated algorithm is recognized, otherwise false is returned.
     */
    bool negotiatedCryptoC2s(Botan::SecureVector<Botan::byte>& cryptoAlgo);

    /**
     * Parses negotiated server to client cipher algorithm and registers it's integer representation with the class.
     * @param cryptoAlgo Reference to a vector containing the negotiated cipher algorithm.
     * @return True if negotiated algorithm is recognized, otherwise false is returned.
     */
    bool negotiatedCryptoS2c(Botan::SecureVector<Botan::byte>& cryptoAlgo);

    /**
     * Parses negotiated client to server HMAC algorithm and registers it's integer representation with the class.
     * @param macAlgo Reference to a vector containing the negotiated HMAC algorithm.
     * @return True if negotiated algorithm is recognized, otherwise false is returned.
     */
    bool negotiatedMacC2s(Botan::SecureVector<Botan::byte>& macAlgo);

    /**
     * Parses negotiated server to client HMAC algorithm vector and registers it's integer representation with the class.
     * @param macAlgo Reference to a vector containing the negotiated HMAC algorithm.
     * @return True if negotiated algorithm is recognized, otherwise false is returned.
     */
    bool negotiatedMacS2c(Botan::SecureVector<Botan::byte>& macAlgo);

    /**
     * Parses negotiated client to server compression algorithm and registers it's integer representation with the class.
     * @param cmprsAlgo Reference to a vector containing the negotiated compression algorithm.
     * @return True if negotiated algorithm is recognized, otherwise false is returned.
     */
    bool negotiatedCmprsC2s(Botan::SecureVector<Botan::byte>& cmprsAlgo);

    /**
     * Parses negotiated server to client compression algorithm and registers it's integer representation with the class.
     * @param cmprsAlgo Reference to a vector containing the negotiated compression algorithm.
     * @return True if negotiated algorithm is recognized, otherwise false is returned.
     */
    bool negotiatedCmprsS2c(Botan::SecureVector<Botan::byte>& cmprsAlgo);

    /**
     * Generates KEX public key.
     * @param publicKey Public key will be dumped into this var.
     * @return True if key generation was successful, otherwise false is returned.
     */
    bool getKexPublic(Botan::BigInt& publicKey);

    /**
     * At the end of key exchange this function is used to generate a shared secret key from private KEX key, that one gets from getKexPublic() function and F value received from a server.
     * @param result Secret key will be dumped into this var.
     * @param f Reference to F value.
     * @return True if key generation was successful, otherwise false is returned.
     */
    bool makeKexSecret(Botan::SecureVector<Botan::byte>& result, Botan::BigInt& f);

    /**
     * Computes H value by checking what hash algorithm is used and hashing "val".
     * @param result H value will be dumped into this var.
     * @param val Reference to vector containing material for H variable generation.
     * @return True if key generation was successful, otherwise false is returned.
     */
    bool computeH(Botan::SecureVector<Botan::byte>& result, Botan::SecureVector<Botan::byte>& val);

    /**
     * Verifies host signature.
     * @param hostKey Reference to vector containing hostKey.
     * @param sig Regerence to vector containing the signature.
     * @return True if signature verification was successful, otherwise false is returned.
     */
    bool verifySig(Botan::SecureVector<Botan::byte>& hostKey, Botan::SecureVector<Botan::byte>& sig);

    /**
     * Generates new cipher and HMAC keys.
     * @return True if key generation was successful, otherwise false is returned.
     */
    bool makeNewKeys();

    /**
     * Encrypts a packet and generates HMAC, if enabled during negotiation.
     * <p>The entire packet is encrypted, only HMAC stays in raw format.
     * @param crypted Encrypted packet will be dumped into this var.
     * @param hmac HMAC will be dumped into this var.
     * @param packet Reference to vector containing unencrypted packet.
     * @param seq Transmited packet sequence.
     * @return True if encryption successful, otherwise false is returned.
     */
    bool encryptPacket(Botan::SecureVector<Botan::byte>& crypted, Botan::SecureVector<Botan::byte>& hmac, Botan::SecureVector<Botan::byte>& packet, uint32 seq);

    /**
     * Decrypts a packet.
     * @param decrypted Decrypted payload will be dumped into this var.
     * @param packet Reference to vector containing encrypted packet.
     * @param len Specifies the length of chunk to be decrypted.
     * @return True if decryption is successful, otherwise false returned.
     */
    bool decryptPacket(Botan::SecureVector<Botan::byte>& decrypted, Botan::SecureVector<Botan::byte>& packet, uint32 len);

    /**
     * Computes HMAC from specific packet.
     * @param hmac Generated HMAC value will be dumped into this var.
     * @param packet Reference to vector containing packet for HMAC generation.
     * @param seq receive sequence.
     */
    void computeMac(Botan::SecureVector<Botan::byte>& hmac, Botan::SecureVector<Botan::byte>& packet, uint32 seq);

    /**
     * Compresses the data.
     * @param buffer Reference to vector containing payload to be compress. Results will also be dumped into this var.
     */
    void compressData(Botan::SecureVector<Botan::byte>& buffer);

    /**
     * Decompresses the data.
     * @param buffer Reference to vector containing packet payload to decompress. Result will also be dumped into this var.
     */
    void decompressData(Botan::SecureVector<Botan::byte>& buffer);

    /**
     * Checks if compression is enabled.
     * @return If compression is enabled returns true, otherwise false is returned.
     */
    bool isCompressed()
    {
        if (_decompress)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    /**
     * Returns private AutoSeededRNG object.
     * @return Point to AutoSeededRNG object.
     */
};

#endif
