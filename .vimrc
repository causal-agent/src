set nocp " No child-porn here :P

" Load pathogen
runtime bundle/vim-pathogen/autoload/pathogen.vim
call pathogen#infect()

" Remap leader to ,
noremap \ ,
let mapleader = ","

" General
set backspace=indent,eol,start
set hidden
set backup
set history=50
set ruler
set showcmd
set incsearch
set ignorecase
set smartcase
set autoindent
set smartindent
set nu
set spell
set cc=80
set title
set shortmess=atI
set visualbell t_vb=
set cursorline
set smarttab
set hlsearch
set list
set listchars=tab:»·,trail:·
set foldmethod=syntax
set foldlevel=1000

set laststatus=2 " Always show statusline
set statusline=%<%f\ %h%m%r%{fugitive#statusline()}%=%-14.(%l,%c%V%)\ %P

syntax on
colorscheme Tomorrow-Night

" Enable mouse in terminals
if has('mouse')
  set mouse=a
endif

" GUI options
set guioptions-=mrLtT " Disable tear-off menus, toolbar, scrollbars
set guifont=Monospace\ 9
set browsedir=buffer " Open dialog starts in working directory
let g:Powerline_symbols = 'fancy'

" Jump to the last cursor position when opening
au BufReadPost * if &filetype !~ '^git\c' && line("'\"") > 0 && line("'\"") <= line("$")
    \| exe "normal! g`\"" | endif

" Default to 2-space indents, 4-character tabs
set expandtab
set shiftwidth=2
set tabstop=4
set shiftround
filetype plugin indent on

" Indentation exceptions
autocmd FileType c,cpp,lua setlocal sw=4
autocmd FileType markdown setlocal tw=72

" C/C++ indent options
" :0  Align case with switch
" l1  Indent case bodies with braces to case
" g0  Align "public:" and friends to class
set cinoptions=:0,l1,g0

" Better tab-complete when opening
set wildmenu
set wildmode=list:longest
set wildignore=*.o,*.d,*~

" Smarter %
runtime macros/matchit.vim

" Disable visible whitespace in insert mode
autocmd InsertEnter * setlocal nolist
autocmd InsertLeave * setlocal list

" Syntastic options
let g:syntastic_check_on_open=1
let g:syntastic_enable_signs=0
let g:syntastic_auto_loc_list=2

" Custom maps
nnoremap ' `
nnoremap ` '

" Clear search highlights
nmap <silent> <leader>n :silent :nohlsearch<CR>

nmap <silent> <leader>s :set list!<CR>

nmap <silent> <leader>z :set spell!<CR>

nmap <leader>p "+p
nmap <leader>P "+P
nmap <leader>y "+y
nmap <leader>Y "+Y
nmap <leader>d "+d
nmap <leader>D "+D

nmap Y y$

" Insert hard tab
imap <silent> <S-tab> <C-v><tab>

nmap Q gq

nmap <silent> <leader>b :CtrlPBuffer<CR>
nmap <silent> <leader>e :CtrlP<CR>

nmap <silent> <leader>gs :Gstatus<CR>
nmap <silent> <leader>gc :Gcommit<CR>
nmap <silent> <leader>gp :Git push<CR>

nnoremap <leader>u :GundoToggle<CR>

" Toggle relative/absolute numbers
function! NumberToggle()
  if(&relativenumber == 1)
    set number
  else
    set relativenumber
  endif
endfunc

nnoremap <silent> <C-n> :call NumberToggle()<cr>

" Custom commands
command! W :w

