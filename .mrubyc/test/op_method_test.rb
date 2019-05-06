# frozen_string_literal: true

class OpMethodTest < MrubycTestCase

  # メソッドで実行される演算子のテスト
  # 　単項論理演算子 ! は、test_boolへ
  #　 ===　演算子は、test_equal3へ
  #　 <=>　演算子は、test_compareへ

  description "+, -"
  def plus_minus_case
    a = 123
    assert_equal( 123, +123 )
    #assert_equal( 123, +a )        # 実装が必要か怪しい
    assert_equal( -123, -a )

    a = 2.72
    assert_equal( 2.72, 2.72 )
    #assert_equal( 2.72, +a )
    assert_equal( -2.72, -a )
  end

  description "剰余"
  def surplus_case
    a = 123
    assert_equal( 3, 123 % 12 )
    assert_equal( 3, a % 12 )
  end

  description "ビット演算"
  def op_bit_case
    a = 0x15a0
    assert_equal( -5537, ~a )

    assert_equal( 0x01, 0x01 << 0 )
    assert_equal( 0x02, 0x01 << 1 )
    assert_equal( 0x04, 0x01 << 2 )
    assert_equal( 0x8000_0000, 0x01 << 31 )
    assert_equal( 0, 0x01 << 32 )   # when 32bit mrbc_int

    assert_equal( 0x4000_0000, 0x4000_0000 >> 0 )
    assert_equal( 0x2000_0000, 0x4000_0000 >> 1 )
    assert_equal( 0, 0x4000_0000 >> 32 )

    assert_equal( 0x0034, 0x1234 & 0x00ff )
    assert_equal( 0x12ff, 0x1234 | 0x00ff )
    assert_equal( 0x12cb, 0x1234 ^ 0x00ff )
  end

  description "== (op_eq)"
  def op_eq_case
    assert_true( 1 == 1 )
    assert_false( 1 == 2 )
    assert_true( 1 == 1.0 )
    assert_true( 1.0 == 1 )
    assert_true( 1.0 == 1.0 )
  end

  description "!="
  def op_not_eq_case
    assert_true( 1 != 2 )
    assert_false( 1 != 1 )
    assert_true( 1 != 2.0 )
    assert_true( 1.0 != 2 )
    assert_true( 1.0 != 2.0 )
  end

  description "&&"
  def op_and_case
    assert_equal( 2,     1 && 2 )
    assert_equal( false, 1 && false )
    assert_equal( nil,   1 && nil )
    assert_equal( false, false && 2 )
    assert_equal( nil,   nil && 2 )
    assert_equal( nil,   nil && false )
  end

  description "||"
  def op_or_case
    assert_equal( 1,     1 || 2 )
    assert_equal( 1,     1 || false )
    assert_equal( 1,     1 || nil )
    assert_equal( 2,     false || 2 )
    assert_equal( 2,     nil || 2 )
    assert_equal( false, nil || false )
  end
end
