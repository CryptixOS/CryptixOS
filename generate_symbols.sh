#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
echo 'generating ksyms.sym...'
tmp=$(mktemp)
NM="${NM:-nm}"
llvm-nm -n Kernel/Cryptix.elf > "$tmp"
printf "%08x
" "$(wc -l "$tmp" | awk '{print $1}')" > ksyms.sym
cat "$tmp" >> ksyms.sym
rm -f "$tmp"
