/*
 *
 * Copyright 2019 Asylo authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef ASYLO_IDENTITY_ATTESTATION_SGX_SGX_AGE_REMOTE_ASSERTION_GENERATOR_H_
#define ASYLO_IDENTITY_ATTESTATION_SGX_SGX_AGE_REMOTE_ASSERTION_GENERATOR_H_

#include <string>
#include <vector>

#include "asylo/crypto/certificate.pb.h"
#include "asylo/crypto/certificate_util.h"
#include "asylo/identity/attestation/enclave_assertion_generator.h"
#include "asylo/identity/attestation/sgx/internal/sgx_remote_assertion_generator_client.h"
#include "asylo/identity/identity.pb.h"
#include "asylo/util/mutex_guarded.h"
#include "asylo/util/status.h"
#include "asylo/util/statusor.h"

namespace asylo {

/// A thread-safe implementation of the EnclaveAssertionGenerator interface for
/// SGX remote assertions generated by the Assertion Generator Enclave (AGE).
///
/// An SgxAgeRemoteAssertionGenerator is capable of generating assertion offers
/// and assertions for SGX identities that can be remotely verified.
class SgxAgeRemoteAssertionGenerator final : public EnclaveAssertionGenerator {
 public:
  /// Constructs an uninitialized SgxAgeRemoteAssertionGenerator.
  ///
  /// The generator can be initialized via a call to Initialize().
  SgxAgeRemoteAssertionGenerator();

  ///////////////////////////////////////////
  //   From AssertionAuthority interface.  //
  ///////////////////////////////////////////

  Status Initialize(const std::string &config) override;

  bool IsInitialized() const override;

  EnclaveIdentityType IdentityType() const override;

  std::string AuthorityType() const override;

  ///////////////////////////////////////////
  //   From AssertionGenerator interface.  //
  ///////////////////////////////////////////

  Status CreateAssertionOffer(AssertionOffer *offer) const override;

  StatusOr<bool> CanGenerate(const AssertionRequest &request) const override;

  Status Generate(const std::string &user_data, const AssertionRequest &request,
                  Assertion *assertion) const override;

 private:
  // The identity type handled by this generator.
  static constexpr EnclaveIdentityType kIdentityType = CODE_IDENTITY;

  // The authority type handled by this generator.
  static const char *const kAuthorityType;

  // Struct that holds class members to be guarded by the initialization mutex.
  struct Members {
    // The root CAs' certificates in X.509 format.
    std::vector<Certificate> root_ca_certificates;

    // The root CAs' certificates as CertificateInterfaces
    CertificateInterfaceVector root_ca_certificate_interfaces;

    // The server address of the Assertion Generator Enclave (AGE).
    std::string server_address;

    // Indicates whether this generator has been initialized.
    bool initialized;

    Members() : initialized(false) {}
  };

  MutexGuarded<Members> members_;
};

}  // namespace asylo

#endif  // ASYLO_IDENTITY_ATTESTATION_SGX_SGX_AGE_REMOTE_ASSERTION_GENERATOR_H_
