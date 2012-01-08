set nocompatible

noremap \ ,
let mapleader = ","

set backspace=indent,eol,start " allow backspacing everything
set hidden
set backup
set history=50
set ruler " show cursor position all the time
set showcmd " show incomplete commands
set incsearch
set ignorecase
set smartcase
set guioptions-=t " no tear-off menus
if has('mouse')
  set mouse=a
endif
if &t_Co > 2 || has("gui_running")
  syntax on
  set hlsearch
endif

augroup vimrcEx
au!
" jump to the last cursor position
autocmd BufReadPost *
  \ if line("'\"") > 1 && line("'\"") <= line("$") |
  \   exe "normal! g`\"" |
  \ endif
augroup END

set autoindent
set smartindent
colorscheme Tomorrow-Night
set nu

" 4-space indents
set tabstop=4
set shiftwidth=4
set expandtab

filetype plugin indent on
autocmd FileType ruby setlocal expandtab shiftwidth=2

"set showmatch " jump to matching bracket
set guioptions-=T " no toolbar in gvim
"set cc=80 " highlight 80th column
set guifont=Monospace\ 9

set spell
"set fdm=syntax " fold by syntax

set cc=80 " Highlight column 80

nnoremap ' `
nnoremap ` '

"better tab complete
set wildmenu
set wildmode=list:longest
set wildignore=*.o,*.d,*~

set title " change terminal title

runtime macros/matchit.vim " smarter %

nmap <silent> <leader>n :silent :nohlsearch<CR>

" Show trailing whitespace with ,s
nmap <silent> <leader>s :set nolist!<CR>

set shortmess=atI

" Easy X copy/paste
map <leader>p "+p
map <leader>P "+P
map <leader>y "+y
map <leader>Y "+Y
map <leader>d "+d
map <leader>D "+D

set visualbell t_vb=

set cursorline

" Insert hard tab
imap <silent> <S-tab> <C-v><tab>

set browsedir=buffer " GUI Open starts in CWD

map Q gq

command DiffOrig vert new | set bt=nofile | r ++edit # | 0d_ | diffthis | wincmd p | diffthis

" Show trailing whitespace and hard tabs
set list
set listchars=tab:»·,trail:·

set smarttab

command W :w " I often accidentally type :W when I mean :w
