# ifrq.awk: the questions of an ifrextractor text dump, one line each:
# form <TAB> varstore <TAB> offset <TAB> size(bits) <TAB> prompt <TAB> options (value=label, ...; * marks the default)
/^\tVarStore Guid:/ {
    match($0, /VarStoreId: (0x[0-9A-F]+)/, a); match($0, /Name: "([^"]*)"/, b); vs[a[1]] = b[1]
}
/^\tForm FormId:/ { match($0, /Title: "([^"]*)"/, t); form = t[1] }
/(OneOf|CheckBox|Numeric) Prompt:/ {
    flush()
    match($0, /Prompt: "([^"]*)"/, p); match($0, /VarStoreId: (0x[0-9A-F]+)/, v); match($0, /VarOffset: (0x[0-9A-F]+)/, o)
    match($0, /Size: ([0-9]+)/, s)
    if (o[1] == "") next
    q = 1; qp = p[1]; qv = vs[v[1]]; qo = o[1]; qs = (s[1] == "" ? 8 : s[1]); opts = ""
    if ($0 ~ /CheckBox/) opts = "0=Disabled, 1=Enabled"
    if ($0 ~ /Numeric/) { match($0, /Min: (0x[0-9A-F]+), Max: (0x[0-9A-F]+)/, m); opts = "numeric " m[1] ".." m[2] }
    next
}
q && /OneOfOption Option:/ {
    match($0, /Option: "([^"]*)" Value: ([0-9]+)/, x)
    opts = opts (opts == "" ? "" : ", ") x[2] "=" x[1] ($0 ~ /Default/ ? "*" : "")
    next
}
q && /^\t+End/ { flush() }
function flush() { if (q) printf "%s\t%s\t%s\t%s\t%s\t%s\n", form, qv, qo, qs, qp, opts; q = 0 }
END { flush() }
