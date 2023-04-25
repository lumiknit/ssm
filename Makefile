.PHONY: all release

release:
	cargo +nightly build --target "$(rustc -vV | grep host | cut -d ' ' -f 2)" --release