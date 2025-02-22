CARGO_COMPILE_COMMANDS = $(BINDIR)/cargo-compile-commands.json
CARGO_COMPILE_COMMANDS_FLAGS = --clang

# This is duplicating the compile-commands rule because unlike in the use case
# when a $(RIOTBASE)/compile_commands.json is built, we *want* this to be
# per-board and per-application. (The large mechanisms are shared anyway).
#
# Changes relative to the compile-commands rule: This uses lazysponge to keep
# Rust from rebuilding, and uses a custom output file and
# CARGO_COMPILE_COMMAND_FLAGS.
$(CARGO_COMPILE_COMMANDS): $(BUILDDEPS)
	$(Q)DIRS="$(DIRS)" APPLICATION_BLOBS="$(BLOBS)" \
	  "$(MAKE)" -C $(APPDIR) -f $(RIOTMAKE)/application.inc.mk compile-commands
	@# replacement addresses https://github.com/rust-lang/rust-bindgen/issues/1555
	$(Q)$(RIOTTOOLS)/compile_commands/compile_commands.py $(CARGO_COMPILE_COMMANDS_FLAGS) $(BINDIR) \
	  | sed 's/"riscv-none-embed"/"riscv32"/g' \
	  | $(LAZYSPONGE) $@


$(CARGO_LIB): $(RIOTBUILD_CONFIG_HEADER_C) $(BUILDDEPS) $(CARGO_COMPILE_COMMANDS) FORCE
	$(Q)command -v cargo >/dev/null || ($(COLOR_ECHO) \
		'$(COLOR_RED)Error: `cargo` command missing to build Rust modules.$(COLOR_RESET) Please install as described on <https://doc.riot-os.org/using-rust.html>.' ;\
		exit 1)
	$(Q)command -v $${C2RUST:-c2rust} >/dev/null || ($(COLOR_ECHO) \
		'$(COLOR_RED)Error: `'$${C2RUST:-c2rust}'` command missing to build Rust modules.$(COLOR_RESET) Please install as described on <https://doc.riot-os.org/using-rust.html>.' ;\
		exit 1)
	$(Q)command -v rustup >/dev/null || ($(COLOR_ECHO) \
		'$(COLOR_RED)Error: `rustup` command missing.$(COLOR_RESET) While it is not essential for building Rust modules, it is the only known way to install the target core libraries (or nightly for -Zbuild-std) needed to do so. If you do think that building should be possible, please edit this file, and file an issue about building Rust modules with the installation method you are using -- later checks in this file, based on rustup, will need to be adjusted for that.' ;\
		exit 1)
	$(Q)[ x"${RUST_TARGET}" != x"" ] || ($(COLOR_ECHO) "$(COLOR_RED)Error: No RUST_TARGET was set for this platform.$(COLOR_RESET) Set FEATURES_REQUIRED+=rust_target to catch this earlier."; exit 1)
	$(Q)# If distribution installed cargos ever grow the capacity to build RIOT, this absence of `rustup` might be OK. But that'd need them to both have cross tools around and cross core libs, none of which is currently the case.
	$(Q)# Ad grepping for "std": We're not *actually* checking for std but more for core -- but rust-stc-$TARGET is the name of any standard libraries that'd be available for that target.
	$(Q)[ x"$(findstring build-std,$(CARGO_OPTIONS))" != x"" ] || \
		(rustup component list $(patsubst %,--toolchain %,$(CARGO_CHANNEL)) --installed | grep 'rust-std-$(RUST_TARGET)$$' --quiet) || \
		($(COLOR_ECHO) \
		'$(COLOR_RED)Error: No Rust libraries are installed for the board'"'"'s CPU.$(COLOR_RESET) Run\n    $(COLOR_GREEN)$$$(COLOR_RESET) rustup target add $(RUST_TARGET) $(patsubst %,--toolchain %,$(CARGO_CHANNEL))\nor set `CARGO_OPTIONS=-Zbuild-std=core`.'; \
		exit 1)
	$(Q)CC= CFLAGS= CPPFLAGS= CXXFLAGS= RIOT_COMPILE_COMMANDS_JSON="$(CARGO_COMPILE_COMMANDS)" RIOT_USEMODULE="$(USEMODULE)" cargo $(patsubst +,,+${CARGO_CHANNEL}) build --target $(RUST_TARGET) `if [ x$(CARGO_PROFILE) = xrelease ]; then echo --release; else if [ x$(CARGO_PROFILE) '!=' xdebug ]; then echo "--profile $(CARGO_PROFILE)"; fi; fi` $(CARGO_OPTIONS)
	$(Q)CC= CFLAGS= CPPFLAGS= CXXFLAGS= \
		RIOT_COMPILE_COMMANDS_JSON="$(CARGO_COMPILE_COMMANDS)" \
		RIOT_USEMODULE="$(USEMODULE)" \
		cargo $(patsubst +,,+${CARGO_CHANNEL}) \
			build \
			--target $(RUST_TARGET) \
			`if [ x$(CARGO_PROFILE) = xrelease ]; then echo --release; else if [ x$(CARGO_PROFILE) '!=' xdebug ]; then echo "--profile $(CARGO_PROFILE)"; fi; fi` \
			$(CARGO_OPTIONS)

$(APPLICATION_RUST_MODULE).module: $(CARGO_LIB) FORCE
	$(Q)# Ensure no old object files persist. These would lead to duplicate
	$(Q)# symbols, or worse, lingering behaivor of XFA entries.
	$(Q)rm -rf $(BINDIR)/$(APPLICATION_RUST_MODULE)/
	$(Q)mkdir -p $(BINDIR)/$(APPLICATION_RUST_MODULE)/

	$(Q)# On cortex-m0 boards like airfy-beacon, the archive contains a
	$(Q)# bin/thumbv6m-none-eabi.o file; the directory must be present for
	$(Q)# ar to unpack it...
	$(Q)mkdir -p $(BINDIR)/$(APPLICATION_RUST_MODULE)/bin/
	$(Q)cd $(BINDIR)/$(APPLICATION_RUST_MODULE)/ && $(AR) x $<
	$(Q)# ... and move them back if any exist, careful to err if anything is duplicate
	$(Q)rmdir $(BINDIR)/$(APPLICATION_RUST_MODULE)/bin/ || (mv -n $(BINDIR)/$(APPLICATION_RUST_MODULE)/bin/* $(BINDIR)/$(APPLICATION_RUST_MODULE)/ && rmdir $(BINDIR)/$(APPLICATION_RUST_MODULE)/bin/)

# create cargo folders
# This prevents cargo inside docker from creating them with root permissions
# (should they not exist), and also from re-building everything every time
# because the .cargo inside is as ephemeral as the build container.
$(shell mkdir -p ~/.cargo/git ~/.cargo/registry)
