# Developer shortcuts. Run `make help`.
VARIANT ?= GVS
.PHONY: help build cross test coverage all-variants package sign verify fuzz lab itest toolchain lint tf-plan clean

help:            ## Show targets
	@grep -E '^[a-z-]+:.*##' $(MAKEFILE_LIST) | awk -F':.*## ' '{printf "  %-14s %s\n", $$1, $$2}'
build:           ## Host build + unit tests for VARIANT (default GVS)
	scripts/build.sh $(VARIANT)
cross:           ## Cross-compile VARIANT for the card CPU (needs toolchain image)
	scripts/build.sh $(VARIANT) --cross
coverage:        ## Build with coverage report
	scripts/build.sh $(VARIANT) --coverage
all-variants:    ## Host build + tests for all 4 cards
	for v in GVS GVL GVXL GVXL-NAM; do scripts/build.sh $$v || exit 1; done
package:         ## Package VARIANT into dist/*.nmcfw
	scripts/package.sh $(VARIANT)
sign:            ## Sign dist/*.nmcfw with the local dev key
	security/sign-firmware.sh local keys/dev-signing-private.pem
verify:          ## Verify every image in dist/
	for f in dist/*.nmcfw; do security/verify-firmware.sh $$f keys/dev-signing-public.pem; done
fuzz:            ## Run fuzzers for 60 s each (needs clang + libFuzzer)
	firmware/tests/fuzz/build-fuzzers.sh 60
toolchain:       ## Build the toolchain Docker image
	docker build -t nmc-toolchain:local docker/toolchain
lab:             ## Start the 4 simulated cards with Docker Compose
	cd docker && docker compose up -d --build
itest:           ## Integration tests against all simulators
	scripts/run-local-lab.sh
shell:           ## Open a shell inside the toolchain container
	docker run --rm -it -v $(PWD):/workspace -w /workspace nmc-toolchain:local bash
lint:            ## pre-commit on all files
	pre-commit run --all-files
tf-plan:         ## terraform plan for the AWS build platform
	cd terraform/environments/build-platform && terraform init && terraform plan
clean:
	rm -rf build dist reports
