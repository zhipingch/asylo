/*
 *
 * Copyright 2018 Asylo authors
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

#include "asylo/test/grpc/client_enclave.h"

#include <memory>
#include <string>

#include "absl/time/time.h"
#include "asylo/enclave.pb.h"
#include "asylo/grpc/auth/enclave_channel_credentials.h"
#include "asylo/grpc/auth/null_credentials_options.h"
#include "asylo/grpc/auth/sgx_local_credentials_options.h"
#include "asylo/test/grpc/client_enclave.pb.h"
#include "asylo/test/grpc/messenger_client_impl.h"
#include "asylo/trusted_application.h"
#include "asylo/util/status.h"
#include "asylo/util/status_macros.h"
#include "include/grpc/impl/codegen/gpr_types.h"
#include "include/grpc/support/time.h"

namespace asylo {
namespace {

const int64_t kDeadlineMicros = absl::Seconds(10) / absl::Microseconds(1);

// Returns an EnclaveCredentialsOptions object for the given |types|, which is a
// repeated field of GrpcCredentialsOptionsType. |self| indicates whether the
// credentials options should be configured for self or peer.
EnclaveCredentialsOptions GetCredentialsOptions(
    const google::protobuf::RepeatedField<int> types, bool self) {
  EnclaveCredentialsOptions options;
  for (int type : types) {
    switch (static_cast<GrpcCredentialsOptionsType>(type)) {
      case NULL_GRPC_CREDENTIALS_OPTIONS:
        options.Add(self ? SelfNullCredentialsOptions()
                         : PeerNullCredentialsOptions());
        break;
      case SGX_LOCAL_GRPC_CREDENTIALS_OPTIONS:
        options.Add(self ? SelfSgxLocalCredentialsOptions()
                         : PeerSgxLocalCredentialsOptions());
        break;
      case UNKNOWN_GRPC_CREDENTIALS_OPTIONS:
      default:
        continue;
    }
  }
  return options;
}

}  // namespace

Status ClientEnclave::Run(const EnclaveInput &input, EnclaveOutput *output) {
  if (!input.HasExtension(client_enclave_input)) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "Input missing client_input extension");
  }
  const ClientEnclaveInput &client_input =
      input.GetExtension(client_enclave_input);

  const std::string &address = client_input.server_address();
  if (address.empty()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "Input must provide a non-empty server address");
  }
  const std::string &rpc_input = client_input.rpc_input();
  if (rpc_input.empty()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "Input must provide a non-empty RPC input");
  }

  const int64_t connection_deadline =
      client_input.has_connection_deadline_milliseconds()
          ? absl::Milliseconds(
                client_input.connection_deadline_milliseconds()) /
                absl::Microseconds(1)
          : kDeadlineMicros;

  std::shared_ptr<::grpc::ChannelCredentials> channel_credentials =
      EnclaveChannelCredentials(
          GetCredentialsOptions(client_input.self_grpc_creds_options(),
                                /*self=*/true)
              .Add(GetCredentialsOptions(client_input.peer_grpc_creds_options(),
                                         /*self=*/false)));

  // Connect a gRPC channel to the server specified in the EnclaveInput.
  std::shared_ptr<::grpc::Channel> channel =
      ::grpc::CreateChannel(address, channel_credentials);
  test::MessengerClient1 client(channel);
  gpr_timespec absolute_deadline =
      gpr_time_add(gpr_now(GPR_CLOCK_REALTIME),
                   gpr_time_from_micros(connection_deadline, GPR_TIMESPAN));
  if (!channel->WaitForConnected(absolute_deadline)) {
    return Status(error::GoogleError::INTERNAL, "Failed to connect to server");
  }

  // Make an RPC to the server and write the response to EnclaveOutput.
  std::string result;
  ASYLO_ASSIGN_OR_RETURN(result, client.Hello(rpc_input));
  output->SetExtension(rpc_result, result);
  return Status::OkStatus();
}

TrustedApplication *BuildTrustedApplication() { return new ClientEnclave(); }

}  // namespace asylo
