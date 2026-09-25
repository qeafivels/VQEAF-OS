#pragma once
// Pure C++11 decision table for an installed QEAPP's menu state. The UI must
// never send an unverified app to the loader; PackageApp checks signatures again.
namespace InstallerMenuPolicy {
enum class Primary { Verify, Install, Update, Open };
constexpr Primary primary(bool installedTab, bool details, bool verified,
                          bool installedMatch, bool newer) {
  return installedTab ? Primary::Open : (!details || !verified ? Primary::Verify :
         (installedMatch ? (newer ? Primary::Update : Primary::Open) : Primary::Install));
}
constexpr bool canOpen(bool installedTab, bool verified,
                       bool installedMatch, bool newer) {
  return verified && (installedTab || (installedMatch && !newer));
}
}
