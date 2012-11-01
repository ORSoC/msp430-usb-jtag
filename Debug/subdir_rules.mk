################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
main.obj: ../main.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/opt/ti/ccsv5/tools/compiler/msp430_4.1.1/bin/cl430" -vmspx --abi=eabi -g --include_path="/opt/ti/ccsv5/ccs_base/msp430/include" --include_path="/opt/ti/ccsv5/tools/compiler/msp430_4.1.1/include" --advice:power=all --define=__MSP430F5510__ --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="main.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

swi2cmst.obj: ../swi2cmst.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/opt/ti/ccsv5/tools/compiler/msp430_4.1.1/bin/cl430" -vmspx --abi=eabi -g --include_path="/opt/ti/ccsv5/ccs_base/msp430/include" --include_path="/opt/ti/ccsv5/tools/compiler/msp430_4.1.1/include" --advice:power=all --define=__MSP430F5510__ --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="swi2cmst.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '

tps65217.obj: ../tps65217.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/opt/ti/ccsv5/tools/compiler/msp430_4.1.1/bin/cl430" -vmspx --abi=eabi -g --include_path="/opt/ti/ccsv5/ccs_base/msp430/include" --include_path="/opt/ti/ccsv5/tools/compiler/msp430_4.1.1/include" --advice:power=all --define=__MSP430F5510__ --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="tps65217.pp" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '


