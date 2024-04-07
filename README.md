# ps4-eboot-dlc-patcher

This is intended to be used on ps5 where dlc fpkgs dont work. It automatically patches the eboot (or other executables) to use custom prx which replaces calls to the `libSceAppContent` library to load dlcs from the base pkg.

![showcase](https://github.com/idlesauce/ps4-eboot-dlc-patcher/assets/148508202/87d5fb21-f442-45b5-bba9-d4cff2e5de2d)

## Tools
- [selfutil-patched](https://github.com/xSpecialFoodx/SelfUtil-Patched) (Make sure you use this, as this is the only reliable version)
- For repacking and extracting the update you can use either [Modded Warfare's Patch Builder](https://www.mediafire.com/file/xw0zn2e0rjaf5k7/Patch_Builder_v1.3.3.zip/file) or if you prefer the cli you can also use [PS4-Fake-PKG-Tools-3.87](https://github.com/CyB1K/PS4-Fake-PKG-Tools-3.87)

## Download
- [Windows 64-bit](https://github.com/idlesauce/ps4-eboot-dlc-patcher/releases/latest/download/ps4-eboot-dlc-patcher-win-x64.exe)

Other os and arch binaries are available [here](https://github.com/idlesauce/ps4-eboot-dlc-patcher/releases/latest), along with a `framework-dependent` version which is cross-platform, but requires the dotnet 8 runtime.
  
## Instructions
1. Extract the update pkg of the game, or if the game is base only or merged base+update, then extract the `Sc0` and `sce_sys` folders along with the executables to patch. The executables you need will most likely be `eboot.bin` and other `.elf` files (most games only use the `eboot.bin`) (it could also be `.prx`, but ignore `.prx` files in the `sce_module` folder)
1. Run selfutil on the executables to decrypt them.
1. For the easiest usage copy all the dlc pkgs and executables into a folder then highlight and drag them onto the `ps4-eboot-dlc-patcher` exe. (You can also drag just the dlc pkgs onto it and enter the executables paths in the menu, or enter all paths as cli arguments)
1. Select patch executable(s) and wait for the patcher to do its thing
1. At the end the patcher will show a list of paths for each dlc marked as with extra data, you'll need to extract the contents of the respective dlc's Image0 into the given folder inside the update's Image0. You have to create the folders listed here even if the dlc pkgs has no files. (This is because the with/without extra data doesnt actually mean it has files, it refers to the dlc pkg type, some games might be smart enough to know which dlcs to call mount on regardless, some dont call it for any dlc, but this status is meant to indicate to the game that this dlc *can* be mounted, so to avoid potentially unexpected behaviour create these folders.)
1. The patcher outputs the patched executables in the same folder as the patcher exe/eboot_patcher_output, copy back the executables into the update folder. Make sure you rename the file back to what it was before you ran selfutil since it changes the extension to .elf! The `dlcldr.prx` always goes into the root of Image0 of the update, even if the exeutable patches wasnt in that folder.
1. Repack the update

## Limitations
- Right now only games that rely on `libSceAppContent` is supported, some newer cross-gen games use `libSceNpEntitlementAccess`, if this is the case the patcher will give an error, support for this library is doable i just havent gotten around to it yet.
- This isnt a limitation of this program, but something to note is that you cannot create update pkgs for remaster type pkgs.

## Credits and thanks
- The biggest thanks to [jocover](https://github.com/jocover) who came up with the idea, without whom i never would've learned this was possible.
- The [OpenOrbis team](https://github.com/OpenOrbis) for creating [the tools that were used to compile the prx](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain).
- [SocraticBliss](https://github.com/SocraticBliss) for creating [ps4_module_loader](https://github.com/SocraticBliss/ps4_module_loader/).
- [maxton](https://github.com/maxton) for creating [LibOrbisPkg](https://github.com/OpenOrbis/LibOrbisPkg) and the [OpenOrbis team](https://github.com/OpenOrbis) for maintaining it.
- [Iced](https://github.com/icedland/iced)
- [spectre.console](https://github.com/spectreconsole/spectre.console)
