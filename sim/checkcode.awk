# very general processing of vvp output to set a return code
# since apparently Verilog running inside vvp does not have
# a way to affect that directly
BEGIN{code=1}
/Program terminate/{code=0}
{print $0}
END{exit(code)}
