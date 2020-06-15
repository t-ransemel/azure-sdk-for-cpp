// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "common/shared_key_policy.hpp"

#include "common/crypt.hpp"
#include "common/storage_url_builder.hpp"

#include <algorithm>
#include <cctype>

namespace Azure { namespace Storage {
  std::string SharedKeyPolicy::GetSignature(const Core::Http::Request& request) const
  {
    std::string string_to_sign;
    string_to_sign += Azure::Core::Http::HttpMethodToString(request.GetMethod()) + "\n";

    const auto& headers = request.GetHeaders();
    for (std::string headerName :
         {"Content-Encoding",
          "Content-Language",
          "Content-Length",
          "Content-MD5",
          "Content-Type",
          "Date",
          "If-Modified-Since",
          "If-Match",
          "If-None-Match",
          "If-Unmodified-Since",
          "Range"})
    {
      auto ite = headers.find(headerName);
      if (ite != headers.end())
      {
        if (headerName == "Content-Length" && ite->second == "0")
        {
          // do nothing
        }
        else
        {
          string_to_sign += ite->second;
        }
      }
      string_to_sign += "\n";
    }

    // canonicalized headers
    const std::string prefix = "x-ms-";
    for (auto ite = headers.lower_bound(prefix);
         ite != headers.end() && ite->first.substr(0, prefix.length()) == prefix;
         ++ite)
    {
      std::string key = ite->first;
      std::string value = ite->second;
      std::transform(key.begin(), key.end(), key.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      });
      string_to_sign += key + ":" + value + "\n";
    }

    // canonicalized resource
    UrlBuilder resourceUrl(request.GetEncodedUrl());
    string_to_sign += "/" + m_credential->AccountName + "/" + resourceUrl.GetPath() + "\n";
    for (const auto& query : resourceUrl.GetQuery())
    {
      std::string key = query.first;
      std::transform(key.begin(), key.end(), key.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      });
      string_to_sign += key + ":" + query.second + "\n";
    }

    // remove last linebreak
    string_to_sign.pop_back();

    return Base64Encode(HMAC_SHA256(string_to_sign, Base64Decode(m_credential->GetAccountKey())));
  }
}} // namespace Azure::Storage