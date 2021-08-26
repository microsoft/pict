#include "ccommon.h"

namespace pictcli_constraints
{

class ConstraintsTokenizer
{
public:
    ConstraintsTokenizer( IN CModel &model, IN std::wstring &constraintsText ) :
        _model( model ), _constraintsText( constraintsText )
    {
        _currentPosition = _constraintsText.begin();
    }

    void Tokenize();

    ~ConstraintsTokenizer()
    {
        cleanUpTokenLists();
    }

    CTokenLists GetTokenLists() { return( _tokenLists ); }

private:
    CModel&                _model;
    std::wstring&          _constraintsText;
    std::wstring::iterator _currentPosition;
    CTokenLists            _tokenLists;

    void parseConstraint      ( IN OUT CTokenList& tokens );
    void parseClause          ( IN OUT CTokenList& tokens );
    void parseCondition       ( IN OUT CTokenList& tokens );
    void parseTerm            ( IN OUT CTokenList& tokens );
    void doPostParseExpansions( IN OUT CTokenList& tokens );

    CFunction*   getFunction();
    RelationType getRelationType();
    LogicalOper  getLogicalOper();
    CValue*      getValue();
    double       getNumber();
    std::wstring getParameterName();
    std::wstring getString  ( IN const std::wstring& terminator );
    void         getValueSet( OUT CValueSet& valueSet );

    void skipWhiteChars();
    wchar_t peekNextChar();
    void movePosition   ( IN int count );
    bool isNextSubstring( IN const std::wstring& text, IN bool dontMoveCursor = false );

    void cleanUpTokenLists();
};

}
