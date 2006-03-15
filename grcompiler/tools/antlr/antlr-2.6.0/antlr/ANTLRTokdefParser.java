package antlr;

/*
 * ANTLR-generated file resulting from grammar tokdef.g
 * 
 * Terence Parr, MageLang Institute
 * with John Lilley, Empathy Software
 * ANTLR Version 2.6.0; 1996-1999
 */
 import java.io.IOException;
import antlr.TokenBuffer;
import antlr.LLkParser;
import antlr.Token;
import antlr.TokenStream;
import antlr.ParserException;
import antlr.NoViableAltException;
import antlr.MismatchedTokenException;
import antlr.SemanticException;
import antlr.collections.impl.BitSet;
public class ANTLRTokdefParser extends antlr.LLkParser
	   implements ANTLRTokdefParserTokenTypes
 {

	public static final String[] _tokenNames = {
		"<0>",
		"EOF",
		"<2>",
		"NULL_TREE_LOOKAHEAD",
		"ID",
		"STRING",
		"ASSIGN",
		"LPAREN",
		"RPAREN",
		"INT",
		"WS",
		"SL_COMMENT",
		"ML_COMMENT",
		"ESC",
		"DIGIT",
		"XDIGIT",
		"VOCAB"
	};
	
	private static final long _tokenSet_0_data_[] = { 2L, 0L };
	public static final BitSet _tokenSet_0 = new BitSet(_tokenSet_0_data_);
	private static final long _tokenSet_1_data_[] = { 50L, 0L };
	public static final BitSet _tokenSet_1 = new BitSet(_tokenSet_1_data_);
	
	
public ANTLRTokdefParser(TokenBuffer tokenBuf) {
  this(tokenBuf,3);
}
protected ANTLRTokdefParser(TokenBuffer tokenBuf, int k) {
  super(tokenBuf,k);
  tokenNames = _tokenNames;
}
public ANTLRTokdefParser(TokenStream lexer) {
  this(lexer,3);
}
protected ANTLRTokdefParser(TokenStream lexer, int k) {
  super(lexer,k);
  tokenNames = _tokenNames;
}
	public final void file(
		ImportVocabTokenManager tm
	) throws ParserException, IOException {
		
		Token  name = null;
		
		try {      // for error handling
			name = LT(1);
			match(ID);
			{
			_loop3:
			do {
				if ((LA(1)==ID||LA(1)==STRING)) {
					line(tm);
				}
				else {
					break _loop3;
				}
				
			} while (true);
			}
		}
		catch (ParserException ex) {
			reportError(ex);
			consume();
			consumeUntil(_tokenSet_0);
		}
	}
	public final void line(
		ImportVocabTokenManager tm
	) throws ParserException, IOException {
		
		Token  s1 = null;
		Token  lab = null;
		Token  s2 = null;
		Token  id = null;
		Token  para = null;
		Token  id2 = null;
		Token  i = null;
		Token t=null; Token s=null;
		
		try {      // for error handling
			{
			if ((LA(1)==STRING)) {
				s1 = LT(1);
				match(STRING);
				s = s1;
			}
			else if ((LA(1)==ID) && (LA(2)==ASSIGN) && (LA(3)==STRING)) {
				lab = LT(1);
				match(ID);
				t = lab;
				match(ASSIGN);
				s2 = LT(1);
				match(STRING);
				s = s2;
			}
			else if ((LA(1)==ID) && (LA(2)==LPAREN)) {
				id = LT(1);
				match(ID);
				t=id;
				match(LPAREN);
				para = LT(1);
				match(STRING);
				match(RPAREN);
			}
			else if ((LA(1)==ID) && (LA(2)==ASSIGN) && (LA(3)==INT)) {
				id2 = LT(1);
				match(ID);
				t=id2;
			}
			else {
				throw new NoViableAltException(LT(1));
			}
			
			}
			match(ASSIGN);
			i = LT(1);
			match(INT);
			
					Integer value = Integer.valueOf(i.getText());
					// if literal found, define as a string literal
					if ( s!=null ) {
						tm.define(s.getText(), value.intValue());
						// if label, then label the string and map label to token symbol also
						if ( t!=null ) {
							StringLiteralSymbol sl =
								(StringLiteralSymbol) tm.getTokenSymbol(s.getText());
							sl.setLabel(t.getText());
							tm.mapToTokenSymbol(t.getText(), sl);
						}
					}
					// define token (not a literal)
					else if ( t!=null ) {
						tm.define(t.getText(), value.intValue());
						if ( para!=null ) {
							TokenSymbol ts = tm.getTokenSymbol(t.getText());
							ts.setParaphrase(
								para.getText()
							);
						}
					}
					
		}
		catch (ParserException ex) {
			reportError(ex);
			consume();
			consumeUntil(_tokenSet_1);
		}
	}
}