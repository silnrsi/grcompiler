package antlr;

/** A CommonAST whose initialization copies hidden token
 *  information from the Token used to create a node.
 */
public class CommonASTWithHiddenTokens extends CommonAST {
	protected CommonHiddenStreamToken hiddenBefore, hiddenAfter; // references to hidden tokens

	public CommonHiddenStreamToken getHiddenAfter() { return hiddenAfter; }
	public CommonHiddenStreamToken getHiddenBefore() { return hiddenBefore; }
public void initialize(Token tok) {
	CommonHiddenStreamToken t = (CommonHiddenStreamToken)tok;
	super.initialize(t);
	hiddenBefore = t.getHiddenBefore();
	hiddenAfter  = t.getHiddenAfter();
}
}