from pygments import token as t
from pygments.lexer import RegexLexer, bygroups, words

# All reserved words — single Pygments Keyword (Swift-style one color for keywords).
_KEYWORDS = (
    "as",
    "auto",
    "break",
    "continue",
    "else",
    "event",
    "false",
    "for",
    "fun",
    "if",
    "import",
    "in",
    "new",
    "none",
    "return",
    "script",
    "self",
    "state",
    "static",
    "super",
    "true",
    "var",
    "while",
    "get",
    "set",
)

# Builtin / common types (Pygments Keyword.Type → e.g. `kt`).
_TYPES = (
    "Actor",
    "Bool",
    "Float",
    "Form",
    "Int",
    "None",
    "ObjectReference",
    "String",
)


class VellumLexer(RegexLexer):
    name = "Vellum"
    aliases = ("vellum", "vel")
    filenames = ("*.vel",)
    mimetypes = ()

    tokens = {
        "root": [
            (r"//.*?$", t.Comment.Single),
            (r"/\*", t.Comment.Multiline, "comment"),
            (r'"', t.String.Double, "string"),
            (r"[-+*/%=<>!&|^~]+|\?", t.Operator),
            (r"\d+\.\d*|\.\d+|\d+", t.Number),
            # Declarations: capture whitespace in groups — Pygments bygroups drops non-captured spans.
            (
                r"\b(static)(\s+)(fun)(\s+)([A-Za-z_][\w]*)",
                bygroups(t.Keyword, t.Text, t.Keyword, t.Text, t.Name.Function),
            ),
            (
                r"\b(fun)(\s+)([A-Za-z_][\w]*)",
                bygroups(t.Keyword, t.Text, t.Name.Function),
            ),
            (
                r"\b(event)(\s+)([A-Za-z_][\w]*)",
                bygroups(t.Keyword, t.Text, t.Name.Function),
            ),
            (
                r"\b(auto)(\s+)(state)(\s+)([A-Za-z_][\w]*)",
                bygroups(t.Keyword, t.Text, t.Keyword, t.Text, t.Name.Class),
            ),
            (
                r"\b(state)(\s+)([A-Za-z_][\w]*)",
                bygroups(t.Keyword, t.Text, t.Name.Class),
            ),
            (
                r"\b(script)(\s+)([A-Za-z_][\w]*)",
                bygroups(t.Keyword, t.Text, t.Name.Class),
            ),
            (r"[{}():;,\[\].]", t.Punctuation),
            (words(_TYPES, prefix=r"\b", suffix=r"\b"), t.Keyword.Type),
            (words(_KEYWORDS, prefix=r"\b", suffix=r"\b"), t.Keyword),
            # PascalCase: type-like names, not immediate call syntax `Name(` (declarations matched above).
            (r"\b([A-Z][\w]*)\b(?!\s*\()", t.Name.Class),
            (r"[A-Za-z_][\w]*", t.Name),
            (r"\s+", t.Text),
        ],
        "comment": [
            (r"[^*/]+", t.Comment.Multiline),
            (r"/\*", t.Comment.Multiline, "#push"),
            (r"\*/", t.Comment.Multiline, "#pop"),
            (r"[*/]", t.Comment.Multiline),
        ],
        "string": [
            (r"\\.", t.String.Escape),
            (r'"', t.String.Double, "#pop"),
            (r'[^\\"]+', t.String.Double),
        ],
    }
