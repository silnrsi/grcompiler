package antlr;

/**
 * <b>SOFTWARE RIGHTS</b>
 * <p>
 * ANTLR 2.6.0 MageLang Institute
 * <p>
 * We reserve no legal rights to the ANTLR--it is fully in the
 * public domain. An individual or company may do whatever
 * they wish with source code distributed with ANTLR or the
 * code generated by ANTLR, including the incorporation of
 * ANTLR, or its output, into commerical software.
 * <p>
 * We encourage users to develop software with ANTLR. However,
 * we do ask that credit is given to us for developing
 * ANTLR. By "credit", we mean that if you use ANTLR or
 * incorporate any source code into one of your programs
 * (commercial product, research project, or otherwise) that
 * you acknowledge this fact somewhere in the documentation,
 * research report, etc... If you like ANTLR and have
 * developed a nice tool with the output, please mention that
 * you developed it using ANTLR. In addition, we ask that the
 * headers remain intact in our source code. As long as these
 * guidelines are kept, we expect to continue enhancing this
 * system and expect to make other tools available as they are
 * completed.
 * <p>
 * The ANTLR gang:
 * @version ANTLR 2.6.0 MageLang Institute
 * @author Terence Parr, <a href=http://www.MageLang.com>MageLang Institute</a>
 * @author <br>John Lilley, <a href=http://www.Empathy.com>Empathy Software</a>
 */
class StringLiteralElement extends GrammarAtom {
	// atomText with quotes stripped and escape codes processed
	protected String processedAtomText;


	public StringLiteralElement(Grammar g, Token t, int autoGenType) {
		super(g, t, autoGenType);
		if (!(g instanceof LexerGrammar)) {
			// lexer does not have token types for string literals
			TokenSymbol ts = grammar.tokenManager.getTokenSymbol(atomText);
			if (ts == null) {
				g.tool.error("Undefined literal: " + atomText, grammar.getFilename(), t.getLine());
			} else {
				tokenType = ts.getTokenType();
			}
		}
		line = t.getLine();

		// process the string literal text by removing quotes and escaping chars
		// If a lexical grammar, add the characters to the char vocabulary
		processedAtomText = new String();
		for (int i = 1; i < atomText.length()-1; i++)
		{
			char c = atomText.charAt(i);
			if (c == '\\') {
				if (i+1 < atomText.length()-1) {
					i++;
					c = atomText.charAt(i);
					switch (c) {
					case 'n' : c = '\n'; break;
					case 'r' : c = '\r'; break;
					case 't' : c = '\t'; break;
					}
				}
			}
			if (g instanceof LexerGrammar) {
				((LexerGrammar)g).charVocabulary.add(c);
			}
			processedAtomText += c;
		}
	}
	public void generate() {
		grammar.generator.gen(this);
	}
	public Lookahead look(int k) {
		return grammar.theLLkAnalyzer.look(k, this);
	}
}