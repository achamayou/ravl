// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ravl/http_client.h"

#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

namespace ravl
{
  SynchronousHTTPClient::SynchronousHTTPClient(
    size_t request_timeout, size_t max_attempts, bool verbose) :
    HTTPClient(request_timeout, max_attempts, verbose)
  {}

  HTTPRequestSetId SynchronousHTTPClient::submit(
    HTTPRequests&& rs, std::function<void(HTTPResponses&&)>&& callback)
  {
    HTTPRequestSetId id = request_sets.size();
    size_t sz = rs.size();

    request_sets.emplace(id, std::move(rs));
    auto [rit, ok] = response_sets.emplace(id, HTTPResponses(sz));

    if (!ok)
      throw std::bad_alloc();

    auto rsit = request_sets.find(id);
    for (size_t i = 0; i < rsit->second.size(); i++)
    {
      auto& request = rsit->second.at(i);
      HTTPResponse response =
        execute_synchronous(request, request_timeout, max_attempts, verbose);
      response_sets[id][i] = response;

      if (response.status != 200)
        throw std::runtime_error(
          fmt::format("unexpected HTTP status {}", response.status));
    }

    HTTPResponses r;
    r.swap(rit->second);
    callback(std::move(r));
    response_sets.erase(rit);

    return id;
  }

  bool SynchronousHTTPClient::is_complete(const HTTPRequestSetId&) const
  {
    return true;
  }

  void SynchronousHTTPClient::erase(const HTTPRequestSetId& id)
  {
    request_sets.erase(id);
    response_sets.erase(id);
  }
}