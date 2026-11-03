# Source provenance

This independent educational program reuses the following files verbatim from an
OpenBF2 source snapshot. The snapshot is identified by the SHA-256 hashes below.
The upstream project is https://github.com/B2BF/OpenBF2.
The original project describes itself as an independent reconstruction of the
Battlefield 2 1.50 engine, not original DICE source.

| Local file | Original relative path | SHA-256 |
| --- | --- | --- |
| original/Vec3.h | Code/BF2/Math/Vec3.h | A7ECCFDBFE3F0607011E7D37CDDD24F7EA5803BCC9476E938E85A91335030415 |
| original/Vec3.cpp | Code/BF2/Math/Vec3.cpp | 15729E7F71F56C4B7316959CC8B48F9CC9BDD68EE7A0CBCC5CEEA5B3240B2129 |
| original/BuildConfig.h | Code/BF2/Core/BuildConfig.h | 417EA45BA45C35D339A12111F379089FC596A32A24370BADE13D4C0936224FC0 |

The checkout's LICENSE is included verbatim. Original comments are preserved.
New educational sources in this directory are provided under the same GPL-3.0 license.

The new code is a learning aid, not recovered engine behavior. No original binaries,
game assets, credentials, network services, or engine runtime are required.
The original Math translation unit is compiled as x86 C++14 with the explicit
DEDICATED=0 image define and force-included BuildConfig.h; this does not imply that
the learning aid builds the Battlefield client image or reproduces its ABI.
