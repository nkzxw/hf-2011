object AboutFrm: TAboutFrm
  Left = 195
  Top = 202
  BorderStyle = bsDialog
  Caption = #38364#26044'PE Encrypt v1.0 :: Delphi Design'
  ClientHeight = 225
  ClientWidth = 324
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = #32048#26126#39636
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 12
  object Label1: TLabel
    Left = 48
    Top = 8
    Width = 210
    Height = 12
    Caption = #12304'PE Encrypt v1.0'#12305':: Delphi Design'
  end
  object Label2: TLabel
    Left = 64
    Top = 32
    Width = 186
    Height = 12
    Caption = 'Copyright (C) 2004-2005 Liwuyue'
  end
  object Label3: TLabel
    Left = 64
    Top = 56
    Width = 174
    Height = 12
    Cursor = crHandPoint
    Caption = 'http://www.programmerlife.com'
    Font.Charset = ANSI_CHARSET
    Font.Color = clBlue
    Font.Height = -12
    Font.Name = #32048#26126#39636
    Font.Style = []
    ParentFont = False
    OnClick = Label3Click
    OnMouseEnter = Label3MouseEnter
    OnMouseLeave = Label4MouseLeave
  end
  object Label4: TLabel
    Left = 88
    Top = 72
    Width = 120
    Height = 12
    Cursor = crHandPoint
    Caption = 'smokingroom@sina.com'
    Font.Charset = ANSI_CHARSET
    Font.Color = clBlue
    Font.Height = -12
    Font.Name = #32048#26126#39636
    Font.Style = []
    ParentFont = False
    OnClick = Label4Click
    OnMouseEnter = Label3MouseEnter
    OnMouseLeave = Label4MouseLeave
  end
  object Bevel1: TBevel
    Left = 4
    Top = 78
    Width = 318
    Height = 17
    Shape = bsBottomLine
  end
  object Label5: TLabel
    Left = 8
    Top = 104
    Width = 313
    Height = 73
    AutoSize = False
    Caption = 
      #26412#31243#24207#37319#29992'Delphi 6.0'#32232#23531', '#20860#26377#23569#37327'asm'#20195#30908', '#29992#26044#21487#22519#34892#25991#20214#30340#21152#23494'. '#20316#32773#22312'['#22823#23500#32705#35542#22727']'#30340'ID'#28858'smokingr' +
      'oom, '#24076#26395#33021#21644#22823#23478#20132#27969'^_^'#13#10#22914#38656#20659#25773', '#35531#20445#30041#20316#21697#30340#23436#25972#24615'!'#13#10#21443#32771#20316#21697': '#32645#32880#20808#29983#30340'[LC Crypto :: v0.' +
      '1], '#20027#38913'http://www.LuoCong.com, '#22312#27492#34920#31034#24863#35613'!'
    WordWrap = True
  end
  object Image1: TImage
    Left = 8
    Top = 8
    Width = 33
    Height = 33
    Picture.Data = {
      055449636F6E0000010001002020040000000000E80200001600000028000000
      2000000040000000010004000000000000020000000000000000000000000000
      0000000000000000000080000080000000808000800000008000800080800000
      C0C0C000808080000000FF0000FF000000FFFF00FF000000FF00FF00FFFF0000
      FFFFFF0000000000000000000000000000000000000000000000077000000000
      00000000000000000000F7870000000000000000000000077008078700000000
      0000000000000887770087870000000000000000000008F7878F778700000000
      00008000000008F78708F787000000000008B330000000078700078700000000
      0800BB80000000878700878700000000833BB80000008F77878F778700000000
      8BBB8030000008F7878F7787000000800BB80300000000078708F78700000833
      BB803000000000878708F78800008BBBB803000000008F77878F77F800008BBB
      8030000000008F7788F77F778000BBB803000000000008F78F77F000000BBB80
      30000000000008F8F7700888888BB8030000000000008F8F77088BBBBBBB8030
      000000000008F78F708BBBBBBBB7B30000000000008F778F70BBBB7B7B7B7300
      0000000008F77F8F0BBBB0B0B7B7B300000000008F77F78F8BBBBB0B0B7B7300
      000000008F777F788BBBBBB0B0B7B300000000008F77F7008BBB880B0B0B7300
      000000008F77708F8BB00770B0B73000000000008F77F0878BB00070BBB73000
      0000000008F7708808B00800BB73000000000000008F77000788000BB3300000
      000000000008FFF0878077773000000000000000000088808880888800000000
      0000000000000000800080000000000000000000000000000000000000000000
      00000000FFF9FFFFFFF0FFFFFE607FFFFC007FFFF8007FFFF8007FF1F8007FE0
      FC107F80F8007F00F0007F00F8007C01FC007803F8007007F000700FF000201F
      F800003FF800007FF00000FFE00001FFC00001FF800001FF000001FF000001FF
      000001FF000803FF000403FF800207FFC0000FFFE0001FFFF0107FFFFE73FFFF
      FF07FFFF}
  end
  object Bevel2: TBevel
    Left = 4
    Top = 169
    Width = 318
    Height = 17
    Shape = bsBottomLine
  end
  object BtnOk: TButton
    Left = 128
    Top = 192
    Width = 75
    Height = 22
    Caption = #30906#23450'(&O)'
    ModalResult = 1
    TabOrder = 0
  end
end
