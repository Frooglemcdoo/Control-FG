#pragma once
// Per-shader identities guard the frame-dependent offset in the native ray-march start.
inline constexpr size_t phaseMultiplierWord[] = {6};
inline constexpr uint32_t scattering1Instruction[] = {0x9000032, 0x100082, 0x1, 0x10003a, 0x2, 0x4001, 0x3e800000, 0x10003a, 0x1};
inline constexpr unsigned char scattering1Checksum[] = {0xfe, 0xc3, 0xd4, 0x19, 0x4a, 0x26, 0xe9, 0xc9, 0x5e, 0x77, 0x3e, 0x4e, 0x1, 0xfd, 0x3e, 0x38};
inline constexpr uint32_t scattering3Instruction[] = {0x9000032, 0x100082, 0x1, 0x10003a, 0x2, 0x4001, 0x3e800000, 0x10003a, 0x1};
inline constexpr unsigned char scattering3Checksum[] = {0x66, 0x69, 0x8c, 0xdb, 0x2, 0x33, 0x64, 0x8c, 0xf6, 0xb9, 0xb9, 0xee, 0x6f, 0xcc, 0x5, 0x21};
inline constexpr uint32_t scattering5Instruction[] = {0x9000032, 0x100082, 0x1, 0x10003a, 0x2, 0x4001, 0x3e800000, 0x10003a, 0x1};
inline constexpr unsigned char scattering5Checksum[] = {0x2d, 0xf7, 0x6a, 0xef, 0x25, 0xbe, 0x2b, 0x56, 0xb2, 0x32, 0x39, 0xbb, 0xc9, 0xb2, 0xed, 0x34};
inline constexpr uint32_t scattering7Instruction[] = {0x9000032, 0x100082, 0x1, 0x10003a, 0x2, 0x4001, 0x3e800000, 0x10003a, 0x1};
inline constexpr unsigned char scattering7Checksum[] = {0x62, 0x9b, 0x94, 0x92, 0xb3, 0x80, 0x7e, 0x22, 0x61, 0x1b, 0x32, 0x5e, 0x75, 0xd8, 0x87, 0x5b};
inline constexpr ShaderPatch scatteringPatches[] = {
    {"scattering1", 15448, "b79df408a50822ad599b36d61e2e82dfcf830708e0c7ffcf72424988516a53ca", "003e0f78866bce802afca1af9889e1a57b8e71adb382c96caf92b70c1348764a", 10212, scattering1Instruction, 9, phaseMultiplierWord, 1, scattering1Checksum},
    {"scattering3", 15448, "ec991e9c91cbeb82293fcd98ba438d437998ced874e4329b293bf1328ff92254", "900412bf61598eeaedb7d5f3a3321898dfe33d470b7a95b8bf508148ef81d61c", 10212, scattering3Instruction, 9, phaseMultiplierWord, 1, scattering3Checksum},
    {"scattering5", 15448, "2c636c890dd5ceeea28371ad39be6a22404d32b266ca16d8965a77b2c879728f", "fed328ecc013e0b901f4a52c93b5078c28cf1f1734c23a6752620e0f3b8cc2f5", 10212, scattering5Instruction, 9, phaseMultiplierWord, 1, scattering5Checksum},
    {"scattering7", 15448, "ef081264f836e9453a0dc556156a33dd9cbfbf523ec6fbb49d069a2eda219c10", "b39fe427ed20a8782d225db09f97b59e9fc9cdca437a8b7b2010345545d7db98", 10212, scattering7Instruction, 9, phaseMultiplierWord, 1, scattering7Checksum},
};
inline constexpr uint32_t viewVolume0Instruction[] = {0x9000032, 0x100082, 0x0, 0x10003a, 0x0, 0x4001, 0x3f1e377a, 0x10000a, 0x0};
inline constexpr unsigned char viewVolume0Checksum[] = {0x8e, 0xa7, 0x5d, 0x9d, 0xa2, 0x5c, 0xd6, 0x98, 0x66, 0x5d, 0xf4, 0x5c, 0x1f, 0x6a, 0x52, 0xf2};
inline constexpr uint32_t viewVolume1Instruction[] = {0x9000032, 0x100082, 0x0, 0x10003a, 0x0, 0x4001, 0x3f1e377a, 0x10000a, 0x0};
inline constexpr unsigned char viewVolume1Checksum[] = {0x46, 0xa1, 0x21, 0x55, 0x63, 0xca, 0xcb, 0xf0, 0x9d, 0xa6, 0x15, 0x16, 0x8c, 0x73, 0xd9, 0x2e};
inline constexpr uint32_t viewVolume2Instruction[] = {0x9000032, 0x100012, 0x0, 0x10000a, 0x0, 0x4001, 0x3f1e377a, 0x10000a, 0x1};
inline constexpr unsigned char viewVolume2Checksum[] = {0x55, 0x74, 0xfa, 0xa1, 0x4f, 0x61, 0xb3, 0xcd, 0x67, 0x70, 0xd7, 0xde, 0xf7, 0x5, 0xdc, 0x95};
inline constexpr uint32_t viewVolume3Instruction[] = {0x9000032, 0x100012, 0x0, 0x10000a, 0x0, 0x4001, 0x3f1e377a, 0x10000a, 0x1};
inline constexpr unsigned char viewVolume3Checksum[] = {0x29, 0xb0, 0xe9, 0xdf, 0x7e, 0x88, 0xa0, 0xef, 0x23, 0xc7, 0x8c, 0x57, 0x2f, 0x34, 0xc8, 0x0};
inline constexpr uint32_t viewVolume4Instruction[] = {0x9000032, 0x100082, 0x0, 0x10003a, 0x0, 0x4001, 0x3f1e377a, 0x10000a, 0x0};
inline constexpr unsigned char viewVolume4Checksum[] = {0xae, 0x3d, 0xaf, 0xd0, 0x94, 0xb6, 0x18, 0xf2, 0x74, 0xf1, 0x34, 0x2f, 0x5c, 0x1a, 0xe2, 0x7};
inline constexpr uint32_t viewVolume5Instruction[] = {0x9000032, 0x100012, 0x0, 0x10000a, 0x0, 0x4001, 0x3f1e377a, 0x10000a, 0x1};
inline constexpr unsigned char viewVolume5Checksum[] = {0xc3, 0xee, 0x7a, 0x49, 0x34, 0x29, 0x3f, 0x5c, 0x44, 0x58, 0x74, 0x9e, 0x42, 0x6c, 0xa3, 0x1c};
inline constexpr ShaderPatch viewVolumePatches[] = {
    {"viewVolume0", 30584, "8084835a51a193f645df08e8c9f1ad93ba4088f62546d5297d2c42c42fde6322", "048bda568f47d52ea68ed5f052416a645cd08aad643cf2601d924d6bffdef89e", 10420, viewVolume0Instruction, 9, phaseMultiplierWord, 1, viewVolume0Checksum},
    {"viewVolume1", 32556, "2a0c5a9a9b1d89e455adcfe86b89ea282cd526dcf4be834363fb046add0e01d7", "c9c70c5b86a8971d57556a58480255b66df77d01739b8ba87b7296ef1c50b92d", 10508, viewVolume1Instruction, 9, phaseMultiplierWord, 1, viewVolume1Checksum},
    {"viewVolume2", 31532, "78e61089b6cafc28aeda296fadc258c4b413d4fecb2f5517819c2d7edb00adbb", "2e1b2e499b0880939734c8618aef6221dafef9c332addf3b7ad2943e2d27e257", 12000, viewVolume2Instruction, 9, phaseMultiplierWord, 1, viewVolume2Checksum},
    {"viewVolume3", 33504, "133756d603269175b61a4d7e521085cd3d54116698540d83864f09fac2944ff2", "0a6637e667764b194b568aa8b500ebcb4906276ee35193b0b0dcc3d238639961", 12088, viewVolume3Instruction, 9, phaseMultiplierWord, 1, viewVolume3Checksum},
    {"viewVolume4", 40284, "11a074bdc107028050c5b4068c1a52b3b78dbc04f05e7df195524a995b278e78", "32076f2bccc2bcc1b973eb93a767fe36509c642dcb166606d6466893ba4672e2", 12756, viewVolume4Instruction, 9, phaseMultiplierWord, 1, viewVolume4Checksum},
    {"viewVolume5", 41232, "6bfaf9d4d9014497f4c88e31f8dc0d3a643a10d365347703b42b325860478343", "f33cb6aa5c29ca955d74dd25dc0051838ab60a2452aa3e47eec073926e4476ba", 14336, viewVolume5Instruction, 9, phaseMultiplierWord, 1, viewVolume5Checksum},
};
