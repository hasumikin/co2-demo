# frozen_string_literal: true

class SymbolTest < MrubycTestCase

  description "生成"
  def identity_case
    s = :symbol
    assert_equal :symbol, s
  end

  description "比較"
  def eq_case
    s = :symbol
    assert_equal true, s == :symbol
    assert_equal false, s == :symbol2
    assert_equal false, s != :symbol
    assert_equal true, s != :symbol2
  end

  description "to_sym"
  def to_sym_case
    s = "abc"
    assert_equal :abc, s.to_sym
    assert_not_equal :abc, s
  end

  description "to_s"
  def to_s_case
    s = :symbol
    assert_equal "symbol", s.to_s
    assert_not_equal "symbol", s
  end
end
