set nocompatible

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
set number
set spell
set colorcolumn=80
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
set laststatus=2
syntax on

" Enable mouse in terminals
if has('mouse')
  set mouse=a
endif

" Less clutter in terminals
if !has('gui_running')
  set nospell
endif

" GUI options
set guioptions-=mrLtT " Disable menus, toolbar, scrollbars
set guioptions+=c " Disable GUI dialogs
set guifont=Monospace\ 9
set browsedir=buffer " Open dialog starts in working directory

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

" Indent Compojure correctly
autocmd FileType clojure set lispwords+=GET,POST,PUT,DELETE

" Better tab-complete when opening
set wildmenu
set wildmode=list:longest
set wildignore=*.o,*.d,*~

" Smarter %
runtime macros/matchit.vim

" Disable visible whitespace in insert mode
autocmd InsertEnter * setlocal nolist
autocmd InsertLeave * setlocal list

" Remap leader to ,
noremap \ ,
let mapleader = ","

" Custom maps
nnoremap ' `
nnoremap ` '

nmap <leader>n :nohlsearch<CR>

nmap <leader>s :set list!<CR>

nmap <leader>z :set spell!<CR>

nmap <leader>p "+p
nmap <leader>P "+P
nmap <leader>y "+y
nmap <leader>Y "+Y
nmap <leader>d "+d
nmap <leader>D "+D

nmap Y y$

" Insert hard tab
imap <S-tab> <C-v><tab>

nmap Q gq

" Toggle relative/absolute numbers
function! NumberToggle()
  if(&relativenumber == 1)
    set norelativenumber
  else
    set relativenumber
  endif
endfunc

nmap <C-n> :call NumberToggle()<CR>

command! W :w

" Plugins

call plug#begin('~/.vim/plugged')

Plug 'bling/vim-airline'
set noshowmode
let g:airline_left_sep = ''
let g:airline_right_sep = ''
let g:airline#extensions#hunks#non_zero_only = 1
let g:airline#extensions#whitespace#enabled = 0

Plug 'scrooloose/syntastic'
let g:syntastic_check_on_open=1
let g:syntastic_enable_signs=0
let g:syntastic_auto_loc_list=2

Plug 'mhinz/vim-signify'
let g:signify_vcs_list = ['git']
let g:signify_sign_overwrite = 1
let g:signify_sign_change = '~'

Plug 'kien/ctrlp.vim'
nmap <leader>b :CtrlPBuffer<CR>
nmap <leader>e :CtrlP<CR>
nmap <leader>t :CtrlPBufTag<CR>
nmap <leader>l :CtrlPLine<CR>

Plug 'tpope/vim-fugitive'
nmap <leader>gs :Gstatus<CR>
nmap <leader>gc :Gcommit<CR>
nmap <leader>gp :Git push<CR>

Plug 'junegunn/vim-easy-align'
vnoremap <silent> <Enter> :EasyAlign<Enter>

Plug 'Raynes/refheap.vim'
Plug 'majutsushi/tagbar'
Plug 'mattn/gist-vim'
Plug 'mattn/webapi-vim'
Plug 'programble/itchy.vim'
Plug 'programble/jellybeans.vim'
Plug 'sickill/vim-pasta'
Plug 'tpope/vim-commentary'
Plug 'tpope/vim-markdown'
Plug 'tpope/vim-ragtag'
Plug 'tpope/vim-surround'

call plug#end()

colorscheme jellybeans
