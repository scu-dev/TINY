Scanner error test cases

Run each case with:
  ..\tiny.exe <case-file>.tny

Each .tny file is intentionally small and tests exactly one scanner error.

01_number_followed_by_letter.tny
  Expected key output: ERROR: 2n

02_extra_dot_in_number.tny
  Expected key output: ERROR: 100.E1.2

03_exponent_missing_digits.tny
  Expected key output: ERROR: 6e

04_exponent_sign_missing_digits.tny
  Expected key output: ERROR: 8.9E-

05_dot_without_digits.tny
  Expected key output: ERROR: .

06_unterminated_block_comment.tny
  Expected key output: ERROR: unterminated comment
