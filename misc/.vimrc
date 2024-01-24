   :if &term =~ "xterm"
   :  if has("terminfo")
   :  set t_Co=8
   :  set t_Sf=^[[3%p1%dm
   :  set t_Sb=^[[4%p1%dm
   :  else
   :  set t_Co=8
   :  set t_Sf=^[[3%dm
   :  set t_Sb=^[[4%dm
   :  endif
   :endif


" The following two settings disable autoindent and indentkeys so that vim
" would not try indenting when cutiing and pasting C-language code
set noai
set indentkeys=

set ruler
set shiftwidth=3
set tabstop=3
set expandtab
syntax on
set bg=dark
set ignorecase
set hlsearch
"colorscheme murphy
set textwidth=80

"Show the position of the cursor.
set ruler

"Show matching parenthese.
set showmatch

" sets lists
set list

" shows trail spaces and tabs
set listchars=tab:>-,trail:<

" use vi style and disallow some keys that move the cursor left/right to wrap
" to the previous/next line when the cursor is on the first/last character in
" the line. (default in vim:   set ww="b,s")
set ww=""

"Display a status-line
set statusline=~

"map <C-v> "+gP
"map <C-c> "+y

syntax on   " Enable syntax highlighting
hi Comment     guifg=forestgreen
hi Statement   guifg=blue
hi PreProc     guifg=blue
hi Special     guifg=blue
hi Type        guifg=blue
hi PreCondit   guifg=red
hi Number      guifg=red
hi Constant    guifg=red
hi String      guifg=darkGray
syntax on

map <C-t><up> :tabr<cr>
map <C-t><down> :tabl<cr>
map <C-t><left> :tabp<cr>
map <C-t><right> :tabn<cr>


set autowrite
