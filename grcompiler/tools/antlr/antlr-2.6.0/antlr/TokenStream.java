package antlr;

public interface TokenStream {
	public Token nextToken() throws java.io.IOException;
}