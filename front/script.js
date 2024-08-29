let socket;
let userId = 0;
let username = null; // 默认用户名
let currentChat = 0;  //currentChatUserId
let currentChatType = 'public'; 
let message_id = 0;
let messageCache = {};
let publicCache = [];
document.addEventListener('DOMContentLoaded', function () {
    connectWebSocket();

    document.getElementById('showRegister').addEventListener('click', function() {
        document.getElementById('loginContainer').style.display = 'none';
        document.getElementById('registerContainer').style.display = 'block';
    });
    document.getElementById('showLogin').addEventListener('click', function() {
        document.getElementById('registerContainer').style.display = 'none';
        document.getElementById('loginContainer').style.display = 'block';
    });
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
    document.getElementById('registerForm').addEventListener('submit', function(event) {
        event.preventDefault();
        const username = document.getElementById('newUserName').value;
        const password = document.getElementById('newPassword').value;
        const confirmPassword = document.getElementById('confirmPassword').value;

        if (password !== confirmPassword) {
            alert('Passwords do not match!');
            return;
        }
        const registrationData = {
            username: username,
            password: password
        };
        sendMessage('SIGN_UP', -1, -1, JSON.stringify(registrationData));
    });
    // Handle copy UUID button click
    document.getElementById('copyButton').addEventListener('click', function() {
        const uuidText = document.getElementById('uuidText').textContent;
        navigator.clipboard.writeText(uuidText).then(() => {
            alert('UUID copied to clipboard!');
        }).catch(err => {
            console.error('Failed to copy UUID: ', err);
        });
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
        } else if(chatType === 'public-chat'){
            // 处理公共聊天和群组聊天
            document.getElementById('chatMessages').innerHTML = '';
            document.getElementById('friendsList').style.display = 'none';
            document.getElementById('chatTitle').textContent = this.textContent;
            currentChatType = 'public'; 
            currentChat = 0; 
            displayPublicMessages();
        }
    });
});

    document.getElementById('chatForm').addEventListener('submit', function (e) {
        e.preventDefault();
        const messageInput = document.getElementById('messageInput');
        const message = messageInput.value;
        const timestamp = generateTimeStamp();
        
        const content = {
            username : username,
            text : message,
            timestamp : timestamp
        };
        const content_json = JSON.stringify(content);
        if (message.trim() !== '') {
            if(currentChat != 0 ) {
                const msgObj = {
                    fromUserId: userId,
                    toUserId: currentChat, // 
                    messageType: 'TEXT',
                    messageId: generateMessageId(),
                    content: content_json,
                    timestamp: timestamp
                };
                socket.send(JSON.stringify(msgObj));
                addMessageToCache(messageCache, msgObj, content);
            } else if(currentChat === 0){
                
                const msgObj = {
                    fromUserId: userId,
                    toUserId: currentChat, // 
                    messageType: 'GROUP_CHAT',
                    messageId: generateMessageId(),
                    content: content_json,
                    timestamp: timestamp
                };
                socket.send(JSON.stringify(msgObj));
                addMessageToPublicCache(publicCache, msgObj, content);
            }
            console.log(timestamp);
            addMessageToChat(username, message, timestamp, 'default.png', true);
            messageInput.value = '';
        }
    });

    document.getElementById('fileButton').addEventListener('click', function () {
        document.getElementById('fileInput').click();
    });

    document.getElementById('fileInput').addEventListener('change', function () {
        if (this.files.length > 0) {
            const fileName = this.files[0].name;
            addMessageToChat('你', `发送了文件: ${fileName}`, generateTimeStamp(), 'default.png', true);
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

    socket = new WebSocket('ws://localhost:8080');
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

function pullMessage(userId) {
    sendMessage('PULL_MESSAGE', userId, -1, ' ');
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
    sendMessage('LOGIN', userId, -1, password); // LOGIN type
}
function logout(userId) {
    sendMessage('LOGOUT', userId, -1, 'logout'); // LOGOUT type
}
function handleMessage(msg) {
    if (msg.messageType == 'SIGN_UP_RESPONSE'){
        if (msg.content) {
            //document.getElementById('registerContainer').style.display = 'none';
            //document.getElementById('loginContainer').style.display = 'block';
            document.getElementById('uuidText').textContent = msg.content;
            document.getElementById('uuidDisplay').style.display = 'block';
        } else {
            alert('Registration failed: ');
        }
    }
    else if (msg.messageType === 'LOGIN_RESPONSE') {
        try{
            content = JSON.parse(msg.content);
            if (content.status === 'success') {
                document.getElementById('loginContainer').style.display = 'none';
                document.querySelector('.chat-container').style.display = 'flex';
                username = content.username;
                document.getElementById("username").innerText = username;
                pullMessage(userId);
            } else {
                alert('登录失败 :' + content.reason);
        }
        }catch(e){
            console.error("content 解析失败：", e);
        }
    }
    else if(msg.messageType === 'FRIEND_REQUEST'){
        const fromUserId = parseInt(msg.fromUserId);
        const toUserId = parseInt(msg.toUserId);
        if(userId === toUserId){
            showFriendRequestNotification(fromUserId,toUserId);
        } else {
            console.log("userId != toUserId");
        }
        
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
        addMessageToCache(messageCache, msg, content);
        if(msg.fromUserId === currentChat){
            addMessageToChat(content.username || '未知', content.text,  content.timestmap,'default.png', false);
        }
    } else if (msg.messageType === 'FILE') {
        try{
            content = JSON.parse(msg.content);
        }catch(e){
            console.error("content 解析失败：", e);
        }
        addMessageToChat(content.username || '未知', `发送了文件: ${msg.filename}`,  content.timestmap, 'default.png', false);
    } else if(msg.messageType === 'GROUP_CHAT') {
        try{
            content = JSON.parse(msg.content);
        }catch(e){
            console.error("content 解析失败：", e);
        }
        if(msg.toUserId === currentChat){
            addMessageToChat(content.username || '未知', content.text,  content.timestmap,'default.png', false);
        }        
        addMessageToPublicCache(publicCache, msg, content);
    }
}
function addMessageToPublicCache(publicCache, msg, content) {
    try{
        content = JSON.parse(msg.content);
    }catch(e){
        console.error("content 解析失败：", e);
    }
    publicCache.push({
        fromUserId: msg.fromUserId,
        toUserId: msg.toUserId,
        messageId: msg.messageId,
        username: content.username,
        text: content.text,
        timestamp: msg.timestamp
    });
}
function addMessageToCache(messageCache, msg, content) {
    try{
        content = JSON.parse(msg.content);
    }catch(e){
        console.error("content 解析失败：", e);
    }
    // 将消息添加到发送者的缓存中
    messageCache[msg.fromUserId] = (messageCache[msg.fromUserId] || []).concat({
        fromUserId: msg.fromUserId,
        toUserId: msg.toUserId,
        messageId: msg.messageId,
        username: content.username,
        text: content.text,
        timestamp: msg.timestamp
    });

    // 将消息添加到接收者的缓存中
    messageCache[msg.toUserId] = (messageCache[msg.toUserId] || []).concat({
        fromUserId: msg.fromUserId,
        toUserId: msg.toUserId,
        messageId: msg.messageId,
        username: content.username,
        text: content.text,
        timestamp: msg.timestamp
    });
}

function displayPublicMessages() {
    publicCache.sort((a, b) => a.timestamp- b.timestamp);
    publicCache.forEach(message => {
        if(message.fromUserId == userId) {
            addMessageToChat(
                message.username,
                message.text,
                message.timestamp,
                './default.png',
                true
            );            
        } else {
            addMessageToChat(
                message.username,
                message.text,
                message.timestamp,
                './default.png',
                false
            );              
        }

    });
}
function displayCachedMessages(userId, friendId) {

    let combinedMessages = [];
    // 收集 userId 的消息
    if (messageCache[userId]) {
        messageCache[userId].forEach(message => {
            if (message.toUserId === friendId) {
                combinedMessages.push({
                    ...message,
                    isOwnMessage: true
                });
            }
        });
    }

    // 收集 friendId 的消息
    if (messageCache[friendId]) {
        messageCache[friendId].forEach(message => {
            if (message.toUserId === userId) {
                combinedMessages.push({
                    ...message,
                    isOwnMessage: false
                });
            }
        });
    }

    // 根据 messageId 排序消息
    combinedMessages.sort((a, b) => a.messageId - b.messageId);

    combinedMessages.forEach(message => {
        addMessageToChat(
            message.username,
            message.text,
            message.timestamp,
            './default.png',
            message.isOwnMessage
        );
    });
}
function showFriendRequestNotification(fromUserId, toUserId) {

    const friendRequestNotification = document.getElementById('friendRequestNotification');
    friendRequestNotification.style.display = 'block'; 
    friendRequestNotification.addEventListener('click', function () {
        const accept = confirm(`User ${fromUserId} wants to be friends. Do you accept?`);
        if (accept) {
            sendMessage('FRIEND_REQUEST_RESPONSE', toUserId, fromUserId, 'accept'); //对回应消息来说，toUserId 和 fromUserId 应该反过来
        } else {
            sendMessage('FRIEND_REQUEST_RESPONSE', toUserId, fromUserId, 'reject');
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
            displayCachedMessages(userId, currentChat);
        });
    });

    friendsList.style.display = 'block';
}
function addMessageToChat(username, message, timestamp, avatar, isMe) {
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
    let options = {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
        hour12: false // 使用 24 小时制
    };
    let timestamp_text = new Date(timestamp*1000).toLocaleTimeString('zh-CN', options);
    let timeElement = document.createElement("div");
    timeElement.style.color = "gray";
    timeElement.style.fontSize = "small";
    timeElement.style.textAlign = "center";
    timeElement.textContent = timestamp_text;
    document.getElementById('chatMessages').appendChild(timeElement);
    chatMessages.scrollTop = chatMessages.scrollHeight; 
}

function generateMessageId() {
    return message_id++;
}
function generateTimeStamp() {
    return Math.floor(Date.now() / 1000); 
}