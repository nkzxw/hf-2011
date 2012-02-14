using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace DeGuangTicketsHelper
{
    /// <summary>
    /// 项目名称:德广火车票助手
    /// 公司:深圳市德广信息技术有限公司
    /// 作者:武广敬
    /// 此项目目前开源,所以请注意以下几点
    /// 请保留此版权信息.
    /// 自由再散布（Free Distribution）：允许获得源代码的人可自由再将此源代码散布。
    /// 源代码（Source Code）：程序的可执行文件在散布时，必需以随附完整源代码或是可让人方便的事后取得源代码。
    /// 派生著作（Derived Works）：让人可依此源代码修改后，在依照同一授权条款的情形下再散布。
    /// 原创作者程序源代码的完整性（Integrity of The Author’s Source Code）：意即修改后的版本，需以不同的版本号码以与原始的代码做分别，保障原始的代码完整性。
    /// 不得对任何人或团体有差别待遇（No Discrimination Against Persons or Groups）：开放源代码软件不得因性别、团体、国家、族群等设置限制，但若是因为法律规定的情形则为例外（如：美国政府限制高加密软件的出口）。
    /// 对程序在任何领域内的利用不得有差别待遇（No Discrimination Against Fields of Endeavor）：意即不得限制商业使用。
    /// 散布授权条款（Distribution of License）：若软件再散布，必需以同一条款散布之。
    /// 授权条款不得专属于特定产品（License Must Not Be Specific to a Product）：若多个程序组合成一套软件，则当某一开放源代码的程序单独散布时，也必需要符合开放源代码的条件。
    /// 授权条款不得限制其他软件（License Must Not Restrict Other Software）：当某一开放源代码软件与其他非开放源代码软件一起散布时（例如放在同一光盘），不得限制其他软件的授权条件也要遵照开放源代码的授权。
    /// 授权条款必须技术中立（License Must Be Technology-Neutral）：意即授权条款不得限制为电子格式才有效，若是纸本的授权条款也应视为有效。
    /// </summary>
    static class Program
    {
        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new frmTicketsHelper());
        }
    }
}
