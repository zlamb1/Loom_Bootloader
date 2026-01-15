OUTFILE=$1

shift

rm -f "$OUTFILE"

for hdr in "$@"; do
    echo "#include \"${hdr#include/}\"" >>"$OUTFILE"
done

echo "" >>"$OUTFILE"

cat <<EOF >>"$OUTFILE"
void loom_register_export_symbols (void) {
EOF

grep -hv '^#' "$@" | \
gawk 'BEGIN { i=0 }
      match($0, /EXPORT *\(([A-Za-z0-9_]+)\)/, arr) \
      { print "\tloom_symbol_register(\"" arr[1] "\", " arr[1] ");"; i++ } \
      END { print "\tcompile_assert(" i " <= LOOM_SYMTAB_SIZE, \"Symbol table full.\");" }' \
     >>$OUTFILE

echo "}" >>"$OUTFILE"