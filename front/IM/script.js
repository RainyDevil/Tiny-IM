let socket;
let userId = 0;
let username = '晓兔子'; // 默认用户名
let currentChat = 0;  //currentChatUserId
let currentChatType = 'public'; 
document.addEventListener('DOMContentLoaded', function () {
    connectWebSocket();

    document.getElementById('loginForm').addEventListener('submit', function (e) {
        e.preventDefault();
        const userIdInput = document.getElementById('userId').value;
        const password = document.getElementById('password').value;
        
        if (userIdInput.trim() === '' || password.trim() === '') {
            alert('User ID and Password are required!');
            return;
        }
        userId = parseInt(userIdInput, 10);
        login(userId, password);
        
    });  
    document.getElementById('emojiButton').addEventListener('click', function() {
        const emojiPicker = document.getElementById('emojiPicker');
        emojiPicker.style.display = emojiPicker.style.display === 'block' ? 'none' : 'block';
    });

    // 动态生成表情选择面板
    const emojiPicker = document.getElementById('emojiPicker');
    const emojis = [
        '😊', '😂', '❤️', '😍', '😎', '😭', '😢', '😡', '🥺', '👍', '👎', '😅','😥'
        // 可以添加更多表情
    ];

    emojis.forEach(function(emoji) {
        const emojiSpan = document.createElement('span');
        emojiSpan.className = 'emoji';
        emojiSpan.textContent = emoji;
        emojiSpan.addEventListener('click', function() {
            const messageInput = document.getElementById('messageInput');
            messageInput.value += emoji;
            emojiPicker.style.display = 'none'; // 选择后隐藏表情面板
        });
        emojiPicker.appendChild(emojiSpan);
    });      
   document.querySelectorAll('.chat-item').forEach(item => {
    item.addEventListener('click', function () {
        document.querySelector('.chat-item.selected').classList.remove('selected');
        this.classList.add('selected');

        const chatType = this.getAttribute('data-chat');

        if (chatType === 'friends-list') {
            // 显示好友列表
            requestFriendsList();
        } else {
            // 处理公共聊天和群组聊天
            //document.getElementById('chatMessages').innerHTML = '';
            //document.getElementById('friendsList').style.display = 'none';
            document.getElementById('chatTitle').textContent = this.textContent;
            currentChatType = 'public'; // 公共聊天
            currentChat = 0; // 没有特定的好友
        }
    });
});

    document.getElementById('chatForm').addEventListener('submit', function (e) {
        e.preventDefault();
        const messageInput = document.getElementById('messageInput');
        const message = messageInput.value;
        if (message.trim() !== '') {
            const msgObj = {
                fromUserId: userId,
                toUserId: currentChat, // 在公共聊天中，0表示广播
                messageType: 'TEXT',
                messageId: generateMessageId(),
                content: message,
                timestamp: Math.floor(Date.now() / 1000)
            };
            socket.send(JSON.stringify(msgObj));
            addMessageToChat(username, message, 'me.png', true);
            messageInput.value = '';
        }
    });

    document.getElementById('fileButton').addEventListener('click', function () {
        document.getElementById('fileInput').click();
    });

    document.getElementById('fileInput').addEventListener('change', function () {
        if (this.files.length > 0) {
            const fileName = this.files[0].name;
            addMessageToChat('你', `发送了文件: ${fileName}`, 'me.png', true);
        }
    });
    document.getElementById('addFriendButton').addEventListener('click', function () {
        const friendId = document.getElementById('friendIdInput').value.trim();
        if (friendId) {
            addFriendRequest(parseInt(friendId, 10));
        } else {
            alert('Please enter a valid Friend ID');
        }
    });
});
window.addEventListener('beforeunload', function (event) {
    if (userId && userId !== 0) {  
        // 登出
        logout(userId);
    }
    document.getElementById('status').textContent = '离线'
    if (socket) {
        socket.close();
    }
    event.preventDefault(); 
    event.returnValue = '';  
});
function connectWebSocket() {
    const loadingIndicator = document.getElementById('loading');
    loadingIndicator.classList.add('show');

    socket = new WebSocket('ws://frp.locksfire.top:23662');
    socket.onopen = function () {
        console.log('已连接到WebSocket服务器');
        document.getElementById('status').textContent = '在线';
        loadingIndicator.classList.remove('show'); 
        
    };

    socket.onmessage = function (event) {
        const msg = JSON.parse(event.data);
        handleMessage(msg);
    };

    socket.onclose = function () {
        console.log('与WebSocket服务器断开连接');
        document.getElementById('status').textContent = '离线';
        setTimeout(connectWebSocket, 3000); // 3秒后尝试重新连接
    };

    socket.onerror = function (error) {
        console.error('WebSocket错误:', error);
    };
}


function addFriendRequest(friendId) {
    sendMessage('ADD_FRIEND', userId, friendId, ' '); // ADD_FRIEND 类型
}
function requestFriendsList() {
    // 请求好友列表
    sendMessage('FRIEND_LIST', userId, 0, ' ');
}
function sendMessage(type, fromUserId, toUserId, content) {
    const message = {
        fromUserId: fromUserId,
        toUserId: toUserId,
        messageType: type,
        messageId: generateMessageId(),
        content: content,
        timestamp: Math.floor(Date.now() / 1000) 
    };
    socket.send(JSON.stringify(message));
}
function login(userId, password) {
    sendMessage('LOGIN', userId, 0, password); // LOGIN type
}
function logout(userId) {
    sendMessage('LOGOUT', userId, 0, 'logout'); // LOGOUT type
}
function handleMessage(msg) {
    if (msg.messageType === 'LOGIN_RESPONSE') {
        if (msg.content === 'success') {
            document.getElementById('loginContainer').style.display = 'none';
            document.querySelector('.chat-container').style.display = 'flex';
            
        } else {
            alert('登录失败 :' + msg.content);
        }
    } else if(msg.messageType === 'FRIEND_REQUEST'){
        const friendId = parseInt(msg.content);
        showFriendRequestNotification(friendId);
    } else if(msg.messageType === 'FRIEND_LIST'){
        const content = JSON.parse(msg.content);
        console.log(content);
        updateFriendsList(content);
    } else if (msg.messageType === 'TEXT') {
        try{
            content = JSON.parse(msg.content);
        }catch(e){
            console.error("content 解析失败：", e);
        }
        addMessageToChat(content.userName || '未知', content.text, 'default.png', false);
    } else if (msg.messageType === 'FILE') {
        try{
            content = JSON.parse(msg.content);
        }catch(e){
            console.error("content 解析失败：", e);
        }
        addMessageToChat(content.userName || '未知', `发送了文件: ${msg.fileName}`, 'default.png', false);
    }
}

function showFriendRequestNotification(fromUserId) {
    // 显示好友请求通知
    const friendRequestNotification = document.getElementById('friendRequestNotification');
    friendRequestNotification.style.display = 'block'; 

    friendRequestNotification.addEventListener('click', function () {
        // 弹出接受或拒绝的对话框
        const accept = confirm(`User ${fromUserId} wants to be friends. Do you accept?`);
        if (accept) {
            // 发送接受好友请求的消息
            sendMessage('FRIEND_REQUEST_RESPONSE', userId, fromUserId, 'accept');
        } else {
            // 发送拒绝好友请求的消息
            sendMessage('FRIEND_REQUEST_RESPONSE', userId, fromUserId, 'reject');
        }
        // 隐藏通知
        friendRequestNotification.style.display = 'none';
    });
}

function updateFriendsList(friends) {
    const friendsList = document.getElementById('friendsList');
    friendsList.innerHTML = ''; // 清空列表
    friends.forEach(friend => {
        const friendItem = document.createElement('li');
        friendItem.className = 'chat-item';
        friendItem.textContent = friend.name;
        friendItem.setAttribute('data-chat', 'friend');
        friendItem.setAttribute('data-user-id', friend.id); // 设置好友ID
        friendsList.appendChild(friendItem);

        // 添加事件监听器
        friendItem.addEventListener('click', function () {
            // 当用户点击好友时更新聊天界面
            document.querySelector('.chat-item.selected').classList.remove('selected');
            this.classList.add('selected');
            document.getElementById('chatTitle').textContent = friend.name;
            document.getElementById('chatMessages').innerHTML = '';
            currentChatType = 'friend';
            currentChat = friend.id; // 设置当前聊天对象为好友ID
        });
    });

    friendsList.style.display = 'block';
}
function addMessageToChat(username, message, avatar, isMe) {
    const chatMessages = document.getElementById('chatMessages');
    const messageDiv = document.createElement('div');
    messageDiv.className = `message ${isMe ? 'sent' : 'received'}`;
    
    const avatarImg = document.createElement('img');
    avatarImg.src = avatar;
    avatarImg.className = 'avatar';

    const messageContent = document.createElement('div');
    messageContent.className = 'message-content';

    const messageText = document.createElement('div');
    messageText.className = 'message-text';
    messageText.innerHTML = `<strong>${username}:</strong> ${message}`;

    messageContent.appendChild(messageText);
    messageDiv.appendChild(messageContent);
    if (isMe) {
        messageDiv.appendChild(avatarImg);
    } else {
        messageDiv.insertBefore(avatarImg, messageContent);
    }
    chatMessages.appendChild(messageDiv);

    chatMessages.scrollTop = chatMessages.scrollHeight; 
}

function generateMessageId() {
    return Date.now();
}
