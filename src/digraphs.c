/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
 *                                                                             *
 * sc-im is a spreadsheet program that is based on sc. The original authors    *
 * of sc are James Gosling and Mark Weiser, and mods were later added by       *
 * Chuck Martin.                                                               *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright           *
 *    notice, this list of conditions and the following disclaimer.            *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. All advertising materials mentioning features or use of this software    *
 *    must display the following acknowledgement:                              *
 *    This product includes software developed by Andrés Martinelli            *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés Martinelli nor the                        *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRES MARTINELLI ''AS IS'' AND ANY            *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRES MARTINELLI BE LIABLE FOR ANY           *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file digraphs.c
 * \author V. Guruprasad <prasad@inspiredresearch.com>
 * \date 2019-10-02
 * \brief A simple implementation of digraphs like in Vim
 */

#include <wchar.h>
#include <wctype.h>

wint_t digraphs[][2] = {
    { L'a', L'α' }, { L'b', L'β' }, { L'c', L'ξ' }, { L'd', L'δ' },
    { L'e', L'ε' }, { L'f', L'φ' }, { L'g', L'γ' }, { L'h', L'θ' },
    { L'i', L'ι' }, { L'j', L'ϊ' }, { L'k', L'κ' }, { L'l', L'λ' },
    { L'm', L'μ' }, { L'n', L'ν' }, { L'o', L'ο' }, { L'p', L'π' },
    { L'q', L'ψ' }, { L'r', L'ρ' }, { L's', L'σ' }, { L't', L'τ' },
    { L'u', L'υ' }, { L'v', L'ϋ' }, { L'w', L'ω' }, { L'x', L'χ' },
    { L'y', L'η' }, { L'z', L'ζ' },

    { L'A', L'Α' }, { L'B', L'Β' }, { L'C', L'Ξ' }, { L'D', L'Δ' },
    { L'E', L'Ε' }, { L'F', L'Φ' }, { L'G', L'Γ' }, { L'H', L'Θ' },
    { L'I', L'Ι' }, { L'J', L'Ϊ' }, { L'K', L'Κ' }, { L'L', L'Λ' },
    { L'M', L'Μ' }, { L'N', L'Ν' }, { L'O', L'Ο' }, { L'P', L'Π' },
    { L'Q', L'Ψ' }, { L'R', L'Ρ' }, { L'S', L'Σ' }, { L'T', L'Τ' },
    { L'U', L'Υ' }, { L'V', L'Ϋ' }, { L'W', L'Ω' }, { L'X', L'Χ' },
    { L'Y', L'Η' }, { L'Z', L'Ζ' },
};

wint_t trigraphs[][3] = {
    { L'*', L's', L'ς' },
    { L'R', L'T', L'√' },
    { L'F', L'A', L'∀' },
    { L'T', L'E', L'∃' },
    { L'N', L'B', L'∇' },
    { L'(', L'-', L'∈' },
    { L'-', L')', L'∋' },
    { L'd', L'P', L'∂' },
    { L'I', L'n', L'∫' },
    { L'I', L'o', L'∮' },
    { L'D', L'I', L'∬' },
    { L'*', L'P', L'∏' },
    { L'+', L'Z', L'∑' },
    { L'+', L'-', L'±' },
    { L'-', L'+', L'∓' },
    { L'0', L'(', L'∝' },
    { L'0', L'0', L'∞' },
    { L'-', L'L', L'∟' },
    { L'-', L'V', L'∠' },
    { L'P', L'P', L'∥' },
    { L'A', L'N', L'∧' },
    { L'O', L'R', L'∨' },
    { L')', L'U', L'∪' },
    { L'(', L'U', L'∩' },
    { L')', L'C', L'⊃' },
    { L'(', L'C', L'⊂' },
    { L')', L'_', L'⊇' },
    { L'(', L'_', L'⊆' },
    { L'.', L':', L'∴' },
    { L':', L'.', L'∵' },
    { L'?', L'-', L'≃' },
    { L'!', L'=', L'≠' },
    { L'=', L'3', L'≡' },
    { L'=', L'<', L'≤' },
    { L'>', L'=', L'≥' },
    { L'<', L'*', L'≪' },
    { L'>', L'*', L'≫' },
    { L'!', L'<', L'≮' },
    { L'!', L'>', L'≯' },

    { L'\0', L'\0', L'\0' },
};

wint_t get_digraph(wint_t x, wint_t y) {
    if (y == L'*') {
        if (x >= L'a' && x <= L'z')
            return digraphs[x - L'a'][1];
        if (x >= L'A' && x <= L'Z')
            return digraphs[x - L'A' + 26][1];
    }
    wint_t (*tp)[3] = trigraphs;
    for (; *tp[0] != L'\0'; tp++) {
        if (tp[0][0] == x && tp[0][1] == y)
            return tp[0][2];
    }
    return x;
}
