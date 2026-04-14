# Expected MBE Parameters

One `.json` file per PCM input, documenting what the encoder should
produce when given that PCM. See `../README.md` for the JSON schema.

This subdirectory is committable — JSON files document expected behavior
and do not embed the PCM itself. Licensing is inherited from the
originating PCM, which must be declared in the JSON's `copyright` field.
