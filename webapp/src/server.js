export default class Server{
    constructor(){
        this.endpoint = '/internal';
    }

    call(lua){
        return fetch(this.endpoint, {
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json'
            },
            method: 'POST',
            body: JSON.stringify({ lua })
        })
            .then( res => res.text() )
            .then( res => JSON.parse(res) );
    }
}
