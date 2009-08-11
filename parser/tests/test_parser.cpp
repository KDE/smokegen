#include <QtTest/QtTest>

#define private public
#include "ast.h"
#undef private

#include "parser.h"
#include "rpp/preprocessor.h"
#include "control.h"
#include "dumptree.h"
#include "tokens.h"
#include "parsesession.h"
#include "commentformatter.h"

#include "testconfig.h"

#include <QByteArray>
#include <QDataStream>
#include <QFile>

#include <iostream>
#include <rpp/chartools.h>
#include <rpp/pp-engine.h>


bool hasKind(AST*, AST::NODE_KIND);
AST* getAST(AST*, AST::NODE_KIND, int num = 0);

class TestParser : public QObject
{
  Q_OBJECT

  Control control;
  DumpTree dumper;

public:
  TestParser()
  {
  }

private slots:

  void initTestCase()
  {
  }

  void testSymbolTable()
  {
    NameTable table;
    table.findOrInsert("Ideal::MainWindow", sizeof("Ideal::MainWindow"));
    table.findOrInsert("QMainWindow", sizeof("QMainWindow"));
    table.findOrInsert("KXmlGuiWindow", sizeof("KXmlGuiWindow"));
    QCOMPARE(table.count(), size_t(3));
    const NameSymbol *s = table.findOrInsert("QMainWindow", sizeof("QMainWindow"));
    QCOMPARE(QString(s->data), QString("QMainWindow"));
    QCOMPARE(table.count(), size_t(3));
  }

  ///@todo reenable
//   void testControlContexts()
//   {
//     Control control;
//     const NameSymbol *n1 = control.findOrInsertName("a", 1);
//     int *type1 = new int(1); // don't care much about types
//     control.declare(n1, (Type*)type1);
//
//     control.pushContext();
//     int *type2 = new int(2);
//     const NameSymbol *n2 = control.findOrInsertName("b", 1);
//     control.declare(n2, (Type*)type2);
//
//     QCOMPARE(control.lookupType(n1), (Type*)type1);
//     QCOMPARE(control.lookupType(n2), (Type*)type2);
//
//     control.popContext();
//     QCOMPARE(control.lookupType(n1), (Type*)type1);
//     QCOMPARE(control.lookupType(n2), (Type*)0);
//   }

  void testTokenTable()
  {
    QCOMPARE(token_name(Token_EOF), "eof");
    QCOMPARE(token_name('a'), "a");
    QCOMPARE(token_name(Token_delete), "delete");
  }

///@todo reenable
//   void testLexer()
//   {
//     QByteArray code("#include <foo.h>");
//     TokenStream token_stream;
//     LocationTable location_table;
//     LocationTable line_table;
//     Control control;
//
//     Lexer lexer(token_stream, location_table, line_table, &control);
//     lexer.tokenize(code, code.size()+1);
//     QCOMPARE(control.problem(0).message(), QString("expected end of line"));
//
//     QByteArray code2("class Foo { int foo() {} }; ");
//     lexer.tokenize(code2, code2.size()+1);
//     QCOMPARE(control.problemCount(), 1);    //we still have the old problem in the list
//   }

  void testParser()
  {
    QByteArray clazz("struct A { int i; A() : i(5) { } virtual void test() = 0; };");
    pool mem_pool;
    TranslationUnitAST* ast = parse(clazz, &mem_pool);
    QVERIFY(ast != 0);
    QVERIFY(ast->declarations != 0);
  }
  
  void testManyComparisons()
  {
    //Should not crash
    {
      QByteArray clazz("void test() { if(val < f && val < val1 && val < val2 && val < val3 ){ } }");
      pool mem_pool;
      TranslationUnitAST* ast = parse(clazz, &mem_pool);
      QVERIFY(ast != 0);
      QVERIFY(ast->declarations != 0);
      dumper.dump(ast, lastSession->token_stream);
    }
    {
      QByteArray clazz("void test() { if(val < f && val < val1 && val < val2 && val < val3 && val < val4 && val < val5 && val < val6 && val < val7 && val < val8 && val < val9 && val < val10 && val < val11 && val < val12 && val < val13 && val < val14 && val < val15 && val < val16 && val < val17 && val < val18 && val < val19 && val < val20 && val < val21 && val < val22 && val < val23 && val < val24 && val < val25 && val < val26){ } }");
      pool mem_pool;
      TranslationUnitAST* ast = parse(clazz, &mem_pool);
      QVERIFY(ast != 0);
      QVERIFY(ast->declarations != 0);
    }
  }
  
  void testParserFail()
  {
    QByteArray stuff("foo bar !!! nothing that really looks like valid c++ code");
    pool mem_pool;
    TranslationUnitAST *ast = parse(stuff, &mem_pool);
    QVERIFY(ast->declarations == 0);
    QVERIFY(control.problems().count() > 3);
  }

  void testPartialParseFail() {
    {
    QByteArray method("struct C { Something invalid is here };");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);
    QVERIFY(ast != 0);
    QVERIFY(hasKind(ast, AST::Kind_ClassSpecifier));
    }
    {
    QByteArray method("void test() { Something invalid is here };");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);
    QVERIFY(ast != 0);
    QVERIFY(hasKind(ast, AST::Kind_FunctionDefinition));
    }
    {
    QByteArray method("void test() { {Something invalid is here };");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);
    QVERIFY(ast != 0);
    QVERIFY(hasKind(ast, AST::Kind_FunctionDefinition));
    QVERIFY(ast->hadMissingCompoundTokens);
    }
    {
    QByteArray method("void test() { case:{};");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);
    QVERIFY(ast != 0);
    QVERIFY(hasKind(ast, AST::Kind_FunctionDefinition));
    QVERIFY(ast->hadMissingCompoundTokens);
    }
  }

  void testParseMethod()
  {
    QByteArray method("void A::test() {  }");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);
    QVERIFY(ast != 0);
    QVERIFY(hasKind(ast, AST::Kind_FunctionDefinition));
  }

///@todo reenable
//   void testMethodArgs()
//   {
//     QByteArray method("int A::test(int primitive, B* pointer) { return primitive; }");
//     pool mem_pool;
//     Parser parser(&control);
//     TranslationUnitAST* ast = parser.parse(method.constData(),
// 					   method.size() + 1, &mem_pool);
//     // return type
//     SimpleTypeSpecifierAST* retType = static_cast<SimpleTypeSpecifierAST*>
//       (getAST(ast, AST::Kind_SimpleTypeSpecifier));
//     QCOMPARE((TOKEN_KIND)parser.token_stream.kind(retType->start_token),
// 	    Token_int);
//
//     // first param
//     ParameterDeclarationAST* param = static_cast<ParameterDeclarationAST*>
//       (getAST(ast, AST::Kind_ParameterDeclaration));
//     SimpleTypeSpecifierAST* paramType = static_cast<SimpleTypeSpecifierAST*>
//       (getAST(param, AST::Kind_SimpleTypeSpecifier));
//     QCOMPARE((TOKEN_KIND)parser.token_stream.kind(paramType->start_token),
// 	    Token_int);
//     UnqualifiedNameAST* argName  = static_cast<UnqualifiedNameAST*>
//       (getAST(param, AST::Kind_UnqualifiedName));
//     QCOMPARE(parser.token_stream.symbol(argName->id)->as_string(),
// 	    QString("primitive"));
//
//     // second param
//     param = static_cast<ParameterDeclarationAST*>
//       (getAST(ast, AST::Kind_ParameterDeclaration, 1));
//     UnqualifiedNameAST* argType = static_cast<UnqualifiedNameAST*>
//       (getAST(param, AST::Kind_UnqualifiedName));
//     QCOMPARE(parser.token_stream.symbol(argType->id)->as_string(),
// 	    QString("B"));
//
//     // pointer operator
//     QVERIFY(hasKind(param, AST::Kind_PtrOperator));
//
//     argName = static_cast<UnqualifiedNameAST*>
//       (getAST(param, AST::Kind_UnqualifiedName, 1));
//     QCOMPARE(parser.token_stream.symbol(argName->id)->as_string(),
// 	    QString("pointer"));
//
//   }

  void testForStatements()
  {
    QByteArray method("void A::t() { for (int i = 0; i < 10; i++) { ; }}");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);

    QVERIFY(ast != 0);
    QVERIFY(hasKind(ast, AST::Kind_ForStatement));
    QVERIFY(hasKind(ast, AST::Kind_Condition));
    QVERIFY(hasKind(ast, AST::Kind_IncrDecrExpression));
    QVERIFY(hasKind(ast, AST::Kind_SimpleDeclaration));

    QByteArray emptyFor("void A::t() { for (;;) { } }");
    ast = parse(emptyFor, &mem_pool);
    QVERIFY(ast != 0);
    QVERIFY(hasKind(ast, AST::Kind_ForStatement));
    QVERIFY(!hasKind(ast, AST::Kind_Condition));
    QVERIFY(!hasKind(ast, AST::Kind_SimpleDeclaration));
  }

  void testIfStatements()
  {
    QByteArray method("void A::t() { if (1 < 2) { } }");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);
    QVERIFY(hasKind(ast, AST::Kind_Condition));
    QVERIFY(hasKind(ast, AST::Kind_BinaryExpression));
  }

  void testComments()
  {
    QByteArray method("//TranslationUnitComment\n//Hello\nint A; //behind\n /*between*/\n /*Hello2*/\n class B{}; //behind\n//Hello3\n //beforeTest\nvoid test(); //testBehind");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);

    QCOMPARE(CommentFormatter::formatComment(ast->comments, lastSession), QByteArray("TranslationUnitComment")); //The comments were merged

    const ListNode<DeclarationAST*>* it = ast->declarations;
    QVERIFY(it);
    it = it->next;
    QVERIFY(it);
    QCOMPARE(CommentFormatter::formatComment(it->element->comments, lastSession), QByteArray("Hello\n(behind)"));

    it = it->next;
    QVERIFY(it);
    QCOMPARE(CommentFormatter::formatComment(it->element->comments, lastSession), QByteArray("between\nHello2\n(behind)"));

    it = it->next;
    QVERIFY(it);
    QCOMPARE(CommentFormatter::formatComment(it->element->comments, lastSession), QByteArray("Hello3\nbeforeTest\n(testBehind)"));
  }

  void testComments2()
  {
    QByteArray method("enum Enum\n {//enumerator1Comment\nenumerator1, //enumerator1BehindComment\n /*enumerator2Comment*/ enumerator2 /*enumerator2BehindComment*/};");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);

    const ListNode<DeclarationAST*>* it = ast->declarations;
    QVERIFY(it);
    it = it->next;
    QVERIFY(it);
    const SimpleDeclarationAST* simpleDecl = static_cast<const SimpleDeclarationAST*>(it->element);
    QVERIFY(simpleDecl);

    const EnumSpecifierAST* enumSpec = (const EnumSpecifierAST*)simpleDecl->type_specifier;
    QVERIFY(enumSpec);

    const ListNode<EnumeratorAST*> *enumerator = enumSpec->enumerators;
    QVERIFY(enumerator);
    enumerator = enumerator->next;
    QVERIFY(enumerator);

    QCOMPARE(CommentFormatter::formatComment(enumerator->element->comments, lastSession), QByteArray("enumerator1Comment\n(enumerator1BehindComment)"));

    enumerator = enumerator->next;
    QVERIFY(enumerator);

    QCOMPARE(CommentFormatter::formatComment(enumerator->element->comments, lastSession), QByteArray("enumerator2Comment\n(enumerator2BehindComment)"));
  }

  void testComments3()
  {
    QByteArray method("class Class{\n//Comment\n int val;};");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);

    const ListNode<DeclarationAST*>* it = ast->declarations;
    QVERIFY(it);
    it = it->next;
    QVERIFY(it);
    const SimpleDeclarationAST* simpleDecl = static_cast<const SimpleDeclarationAST*>(it->element);
    QVERIFY(simpleDecl);

    const ClassSpecifierAST* classSpec = (const ClassSpecifierAST*)simpleDecl->type_specifier;
    QVERIFY(classSpec);

    const ListNode<DeclarationAST*> *members = classSpec->member_specs;
    QVERIFY(members);
    members = members->next;
    QVERIFY(members);

    QCOMPARE(CommentFormatter::formatComment(members->element->comments, lastSession), QByteArray("Comment"));
  }

  void testComments4()
  {
    QByteArray method("//TranslationUnitComment\n//Comment\ntemplate<class C> class Class{};");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);

    const ListNode<DeclarationAST*>* it = ast->declarations;
    QVERIFY(it);
    it = it->next;
    QVERIFY(it);
    const TemplateDeclarationAST* templDecl = static_cast<const TemplateDeclarationAST*>(it->element);
    QVERIFY(templDecl);
    QVERIFY(templDecl->declaration);

    //QCOMPARE(CommentFormatter::formatComment(templDecl->declaration->comments, lastSession), QString("Comment"));
  }

  QString preprocess(const QString& contents) {
    rpp::Preprocessor preprocessor;
    rpp::pp pp(&preprocessor);
    return QString::fromUtf8(stringFromContents(pp.processFile("anonymous", contents.toUtf8())));
  }

  void testPreprocessor() {
    rpp::Preprocessor preprocessor;
    //QCOMPARE(preprocess("#define TEST (1L<<10)\nTEST").trimmed(), QString("(1L<<10)"));
    QCOMPARE(preprocess("#define TEST //Comment\nTEST 1").trimmed(), QString("1")); //Comments are not included in macros
    QCOMPARE(preprocess("#define TEST /*Comment\n*/\nTEST 1").trimmed(), QString("1")); //Comments are not included in macros

  }

  void testStringConcatenation()
  {
    rpp::Preprocessor preprocessor;
    QCOMPARE(preprocess("Hello##You"), QString("HelloYou"));
    QCOMPARE(preprocess("#define CONCAT(Var1, Var2) Var1##Var2 Var2##Var1\nCONCAT(      Hello      ,      You     )").simplified(), QString("\nHelloYou YouHello").simplified());
  }

  void testCondition()
  {
    QByteArray method("bool i = (small < big || big > small);");
    pool mem_pool;
    TranslationUnitAST* ast = parse(method, &mem_pool);
    dumper.dump(ast, lastSession->token_stream);
    ///@todo make this work, it should yield something like TranslationUnit -> SimpleDeclaration -> InitDeclarator -> BinaryExpression
  }

  void testNonTemplateDeclaration()
  {
    /*{
      QByteArray templateMethod("template <int> class a {}; int main() { const int b = 1; const int c = 2; a<b|c> d; }");
      pool mem_pool;
      TranslationUnitAST* ast = parse(templateMethod, &mem_pool);
      dumper.dump(ast, lastSession->token_stream);
    }*/

    //int a, b, c, d; bool e;
    QByteArray declaration("void expression() { if (a < b || c > d) {} }");
    pool mem_pool;
    TranslationUnitAST* ast = parse(declaration, &mem_pool);
    dumper.dump(ast, lastSession->token_stream);
  }

  /*void testParseFile()
  {
     QFile file(TEST_FILE);
     QVERIFY(file.open(QFile::ReadOnly));
     QByteArray contents = file.readAll();
     file.close();
     pool mem_pool;
     Parser parser(&control);
     ParseSession session;
     session.setContents(contents);
     TranslationUnitAST* ast = parser.parse(&session);
     QVERIFY(ast != 0);
     QVERIFY(ast->declarations != 0);
   }*/

private:
  ParseSession* lastSession;

  TranslationUnitAST* parse(const QByteArray& unit, pool* mem_pool)
  {
    Parser parser(&control);
    lastSession = new ParseSession();
    lastSession->setContentsAndGenerateLocationTable(tokenizeFromByteArray(unit));
    return  parser.parse(lastSession);
  }



};

struct HasKindVisitor : protected DefaultVisitor
{

  AST::NODE_KIND kind;
  AST* ast;
  int num;

  HasKindVisitor(AST::NODE_KIND kind, int num = 0)
    : kind(kind), ast(0), num(num)
  {
  }

  bool hasKind() const
  {
    return ast != 0;
  }

  void visit(AST* node)
  {
    if (!ast && node) {
      if (node->kind == kind && --num < 0) {
	ast = node;
      }
      else {
	DefaultVisitor::visit(node);
      }
    }
  }
};

bool hasKind(AST* ast, AST::NODE_KIND kind)
{
  HasKindVisitor visitor(kind);
  visitor.visit(ast);
  return visitor.hasKind();
}

AST* getAST(AST* ast, AST::NODE_KIND kind, int num)
{
  HasKindVisitor visitor(kind, num);
  visitor.visit(ast);
  return visitor.ast;
}


#include "test_parser.moc"

QTEST_MAIN(TestParser)
