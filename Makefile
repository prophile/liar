runtime.bc: class.bc gc.bc except.bc hash.bc message.bc sparse_data_map.bc
	llvm-ld -link-as-library -internalize-public-api-file=exports.txt -o $@ $^

class.bc: class.c class.h sparse_data_map.h hash.h
	clang -arch x86_64 -O4 -c -o $@ $<

gc.bc: gc.cpp gc.h
	llvm-g++ -arch x86_64 -O4 -c -o $@ $<

except.bc: except.ll
	llvm-as $< -o=- | opt -std-compile-opts -o=$@

hash.bc: hash.c hash.h
	clang -arch x86_64 -O4 -c -o $@ $<

message.bc: message.ll
	llvm-as $< -o=- | opt -std-compile-opts -o=$@

sparse_data_map.bc: sparse_data_map.c sparse_data_map.h
	clang -arch x86_64 -O4 -c -o $@ $<

clean:
	rm -f *.bc
