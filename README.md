# proof-producer
Proof producer for the =nil; Proof Market https://proof.market.nil.foundation/ 
# How to use
It is needed to have [zkllvm](https://raw.githubusercontent.com/NilFoundation/zkllvm).

To generate proof:

1. Run zkllv assigner to prepare assignment table and circuit
```
${ZKLLVM_BUILD:-build}/bin/assigner/assigner -b ${ZKLLVM_BUILD:-build}/examples/cpp/merkle_tree_sha2_256_cpp_example.ll -i ${ZKLLVM_BUILD:-build}/examples/inputs/merkle_tree_sha2_256.inp -t merkle_tree_sha2_256_assignment.tbl -c merkle_tree_sha2_256_circuit.crct -e pallas
```
2. Run proof generator 
```
./bin/proof-generator/proof-generator --circuit=./merkle_tree_sha2_256_circuit.crt --assignment-table=/balances_tree.tbl --proof=./proof.txt
```

3. Enable/disable proof verification

By default, proof-producer verifies the generated proof, and errors if validation fails - proof file is **NOT**
written to disk in this case. To skip verification (and write the file unconditionally), pass `--skip-verification` flag to the proof-generator.

```
./bin/proof-generator/proof-generator --circuit=./merkle_tree_sha2_256_circuit.crt --assignment-table=/balances_tree.tbl --proof=./proof.txt --skip-verification
```
