#include <parsers/grammar.hpp>
#include <iostream>
#include <fstream>


namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

namespace parsers {
	namespace where {


		template<typename THandler>
		struct build_expr {
			template <typename A, typename B = unused_type, typename C = unused_type>
			struct result { typedef expression_ast<THandler> type; };
			
			expression_ast<THandler> operator()(expression_ast<THandler> const & expr1, operators const & op, expression_ast<THandler> const & expr2) const {
				return expression_ast<THandler>(binary_op<THandler>(op, expr1, expr2));
			}
		};

		template<typename THandler>
		struct build_string {
			template <typename A>
			struct result { typedef expression_ast<THandler> type; };

			template <typename A>
			expression_ast<THandler> operator()(A const & v) const {
				return expression_ast<THandler>(string_value(v));
			}
		};

		template<typename THandler>
		struct build_int {
			template <typename A>
			struct result { typedef expression_ast<THandler> type; };

			//template <typename A>
			expression_ast<THandler> operator()(unsigned int const & v) const {
				return expression_ast<THandler>(int_value(v));
			}
		};

		template<typename THandler>
		struct build_variable {
			template <typename A>
			struct result { typedef expression_ast<THandler> type; };

			//template <typename A>
			expression_ast<THandler> operator()(std::wstring const & v) const {
				return expression_ast<THandler>(variable<THandler>(v));
			}
		};

		template<typename THandler>
		struct build_function {
			template <typename A, typename B>
			struct result { typedef expression_ast<THandler> type; };
			expression_ast<THandler> operator()(std::wstring const name, expression_ast<THandler> const & var) const {
				return expression_ast<THandler>(unary_fun<THandler>(name, var));
			}
		};

		template<typename THandler>
		struct build_function_convert {
			template <typename A, typename B>
			struct result { typedef expression_ast<THandler> type; };
			expression_ast<THandler> operator()(wchar_t const unit, expression_ast<THandler> const & vars) const {
				list_value<THandler> args = list_value<THandler>(vars);
				args += string_value(std::wstring(1, unit));
				return expression_ast<THandler>(unary_fun<THandler>(_T("convert"), args));
			}
		};


		template<typename THandler>
		struct build {

		};


		///////////////////////////////////////////////////////////////////////////
		//  Our calculator grammar
		///////////////////////////////////////////////////////////////////////////
		template <typename THandler, typename Iterator>
		where_grammar<THandler, Iterator>::where_grammar() : where_grammar<THandler, Iterator>::base_type(expression) {
			using qi::_val;
			using qi::uint_;
			using qi::_1;
			using qi::_2;
			using qi::_3;

			boost::phoenix::function<build_expr<THandler> > build_e;
			boost::phoenix::function<build_string<THandler> > build_is;
			boost::phoenix::function<build_int<THandler> > build_ii;
			boost::phoenix::function<build_variable<THandler> > build_iv;
			boost::phoenix::function<build_function<THandler> > build_if;
			boost::phoenix::function<build_function_convert<THandler> > build_ic;

			expression	
					= and_expr											[_val = _1]
						>> *("OR" >> and_expr)							[_val |= _1]
					;
			and_expr
					= not_expr 											[_val = _1]
						>> *("AND" >> cond_expr)						[_val &= _1]
					;
			not_expr
					= cond_expr 										[_val = _1]
						>> *("NOT" >> cond_expr)						[_val != _1]
					;

			cond_expr	
					= (identifier >> op >> identifier)					[_val = build_e(_1, _2, _3) ]
					| (identifier >> "NOT IN" 
						>> '(' >> value_list >> ')')					[_val = build_e(_1, op_nin, _2) ]
					| (identifier >> "IN" >> '(' >> value_list >> ')')	[_val = build_e(_1, op_in, _2) ]
					| ('(' >> expression >> ')')						[_val = _1 ]
					;

			identifier 
					= "str" >> string_literal_ex						[_val = build_is(_1)]
					| (variable_name >> '(' >> list_expr >> ')')		[_val = build_if(_1, _2)]
					| variable_name										[_val = build_iv(_1)]
					| string_literal									[_val = build_is(_1)]
					| qi::lexeme[
						(uint_ >> ascii::alpha)							[_val = build_ic(_2, build_ii(_1))]
						]
					| '-' >> qi::lexeme[
						(uint_ >> ascii::alpha)							[_val = build_if(std::wstring(_T("neg")), build_ic(_2, build_ii(_1)))]
						]
					| number											[_val = build_ii(_1)]
					| '-' >> number										[_val = build_if(std::wstring(_T("neg")), build_ii(_1))]
					;

			list_expr
					= value_list										[_val = _1 ]
					;

			value_list
					= string_literal									[_val = build_is(_1) ]
						>> *( ',' >> string_literal )					[_val += build_is(_1) ]
					|	number											[_val = build_ii(_1) ]
						>> *( ',' >> number ) 							[_val += build_ii(_1) ]
					|	variable_name									[_val = build_is(_1) ]
						>> *( ',' >> variable_name )					[_val += build_is(_1) ]
					;

			op 		= qi::lit("<=")										[_val = op_le]
					| qi::lit("<")										[_val = op_lt]
					| qi::lit("=")										[_val = op_eq]
					| qi::lit("!=")										[_val = op_ne]
					| qi::lit(">=")										[_val = op_ge]
					| qi::lit(">")										[_val = op_gt]
					| qi::lit("le")										[_val = op_le]
					| qi::lit("lt")										[_val = op_le]
					| qi::lit("eq")										[_val = op_eq]
					| qi::lit("ne")										[_val = op_ne]
					| qi::lit("ge")										[_val = op_ge]
					| qi::lit("gt")										[_val = op_gt]
					| qi::lit("like")									[_val = op_like]
					| qi::lit("not like")								[_val = op_not_like]
					;

			number
					= uint_												[_val = _1]
					;
			variable_name
					= qi::lexeme[+(ascii::alpha)						[_val += _1]]
					;
			string_literal
					= qi::lexeme[ '\'' 
							>>  +( ascii::char_ - '\'' )				[_val += _1] 
							>> '\''] 
					;
			string_literal_ex
					= qi::lexeme[ '(' 
							>>  +( ascii::char_ - ')' )					[_val += _1] 
							>> ')'] 
					;

// 					qi::on_error<qi::fail>( expression , std::wcout
// 						<< phoenix::val(_T("Error! Expecting "))
// 						<< _4                               // what failed?
// 						<< phoenix::val(_T(" here: \""))
// 						<< phoenix::construct<std::wstring>(_3, _2)   // iterators to error-pos, end
// 						<< phoenix::val(_T("\""))
// 						<< std::endl
//);
// 					qi::on_error<qi::fail>( expression , std::cout
// 						<< phoenix::val("Error! Expecting ")
// 						<< _4                               // what failed?
// 						<< phoenix::val(" here: \"")
// 						<< phoenix::construct<std::string>(_3, _2)   // iterators to error-pos, end
// 						<< phoenix::val("\"")
// 						<< std::endl
// 						);

			//				<< ("Error! Expecting ")
			//				<< _4                               // what failed?
			//				<< (" here: \"")
			//				<< construct<std::string>(_3, _2)   // iterators to error-pos, end
			//				<< ("\"")
// 			<< std::endl

		}

	}
}

