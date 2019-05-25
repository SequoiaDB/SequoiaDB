namespace SequoiaDB
{
    public interface IConnection
    {
        void Connect();
        void Close();
        bool IsClosed();

        void ChangeConfigOptions(ConfigOptions opts);

        void SendMessage(byte[] msg);
        byte[] ReceiveMessage(bool isBigEndian);
        ByteBuffer ReceiveMessage2(bool isBigEndian);
        byte[] ReceiveSysMessage(int msgSize);
    }
}
